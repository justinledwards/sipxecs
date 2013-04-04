/**
 * Copyright (c) 2013 eZuce, Inc. All rights reserved.
 * Contributed to SIPfoundry under a Contributor Agreement
 *
 * This software is free software; you can redistribute it and/or modify it under
 * the terms of the Affero General Public License (AGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 */
package org.sipfoundry.sipxconfig.mongo;

import java.io.File;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.sipfoundry.sipxconfig.cfgmgt.ConfigManager;
import org.sipfoundry.sipxconfig.common.SimpleCommandRunner;
import org.sipfoundry.sipxconfig.common.UserException;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.springframework.beans.factory.annotation.Required;

import com.mongodb.util.JSON;

public class MongoAdmin {
    private static final Log LOG = LogFactory.getLog(MongoAdmin.class);
    private int m_timeout = 1000;
    private int m_backgroundTimeout = 120000; // can take a while for fresh mongo to init
    private String m_mongoStatusScript;
    private String m_mongoAnalyzerScript;
    private String m_mongoAdminScript;
    private FeatureManager m_featureManager;
    private Map<String, MongoNode> m_nodes;
    private List<String> m_clusterStatus;
    private Map<String, List<String>> m_actions;
    private String m_statusToken;
    private String m_analysisToken;
    private File m_modelFile;
    private SimpleCommandRunner m_actionRunner;
    private ConfigManager m_configManager;

    public void setConfigManager(ConfigManager configManager) {
        m_configManager = configManager;
    }

    public MongoNode getNode(String id) {
        return getNodesByServer(getStatusToken()).get(id);
    }

    public Collection<String> getServers() {
        return getNodesByServer(getStatusToken()).keySet();
    }

    public Collection<MongoNode> getNodes() {
        return getNodesByServer(getStatusToken()).values();
    }

    public List<String> getClusterStatus() {
        getServers();
        return m_clusterStatus;
    }

    public Collection<String> getActions(String server) {
        if (m_actions == null) {
            Map<String, Map<String, List<String>>> actions = getAnalysis(getAnalysisToken(getStatusToken()));
            m_actions = (Map<String, List<String>>) actions.get("required");
        }
        List<String> actions = m_actions.get(server);
        if (actions == null) {
            actions = Collections.emptyList();
        }
        return actions;
    }

    public boolean isInProgress() {
        return m_actionRunner != null && m_actionRunner.isInProgress();
    }

    public String takeAction(String server, String action) {
        if (m_actionRunner == null) {
            m_actionRunner = new SimpleCommandRunner();
        } else if (m_actionRunner.isInProgress()) {
            throw new UserException("Operation still in progress");
        }

        String fqdn = MongoNode.label(server);
        String remote = m_configManager.getRemoteCommand(fqdn);
        StringBuilder cmd = new StringBuilder(remote);
        cmd.append(' ').append(m_mongoAdminScript);
        cmd.append(" --host_port ").append(server);
        cmd.append(' ').append(action);
        String response = runBackgroundOk(m_actionRunner, cmd.toString());
        return response;
    }

    Map<String, MongoNode> getNodesByServer(String statusToken) {
        if (m_nodes == null) {
            Map<String, Object> statusData = getStatus(statusToken);
            m_clusterStatus = (List<String>) statusData.get("cluster");
            Map<String, List<String>> states = (Map<String, List<String>>) statusData.get("states");
            m_nodes = new TreeMap<String, MongoNode>();
            for (Entry<String, List<String>> entry : states.entrySet()) {
                MongoNode node = new MongoNode(entry.getKey(), entry.getValue());
                m_nodes.put(entry.getKey(), node);
            }
        }
        return m_nodes;
    }

    public void setBackgroundTimeout(int backgroundTimeout) {
        m_backgroundTimeout = backgroundTimeout;
    }

    public void setMongoAdminScript(String mongoAdminScript) {
        m_mongoAdminScript = mongoAdminScript;
    }

    Map<String, Object> getStatus(String statusToken) {
        Map<String, Object> parse = (Map<String, Object>) JSON.parse(statusToken);
        return parse;
    }

    Map<String, Map<String, List<String>>> getAnalysis(String analysisToken) {
        Map<String, Map<String, List<String>>> parse = (Map<String, Map<String, List<String>>>) JSON
                .parse(analysisToken);
        return parse;
    }

    String getAnalysisToken(String statusToken) {
        if (m_analysisToken == null) {
            SimpleCommandRunner runner = new SimpleCommandRunner();
            runner.setStdin(statusToken);
            m_analysisToken = run(runner, m_mongoAnalyzerScript);
        }
        return m_analysisToken;
    }

    protected void finalize() throws Throwable {
        // not mandatory it be called, hence in finalize
        m_modelFile.delete();
    };

    String getStatusToken() {
        if (m_statusToken == null) {
            StringBuilder cmd = new StringBuilder(m_mongoStatusScript);
            SimpleCommandRunner runner = new SimpleCommandRunner();
            m_statusToken = run(runner, cmd.toString());
        }
        return m_statusToken;
    }



    String runBackgroundOk(SimpleCommandRunner runner, String cmd) {
        if (!runner.run(StringUtils.split(cmd), m_timeout, m_backgroundTimeout)) {
            return null;
        }
        return getOutput(cmd, runner);
    }

    String run(SimpleCommandRunner runner, String cmd) {
        if (!runner.run(StringUtils.split(cmd), m_timeout)) {
            throw new UserException(cmd + " did not complete in time");
        }
        return getOutput(cmd, runner);
    }

    String getOutput(String cmd, SimpleCommandRunner runner) {
        if (0 != runner.getExitCode()) {
            String err = runner.getStderr();
            if (err != null) {
                err = cmd + " had exit code " + runner.getExitCode();
            }
            if (runner.getStdin() != null) {
                LOG.error(runner.getStdin());
            }
            throw new UserException(err);
        }

        return runner.getStdout();
    }

    @Required
    public void setFeatureManager(FeatureManager featureManager) {
        m_featureManager = featureManager;
    }

    public int getTimeout() {
        return m_timeout;
    }

    public void setTimeout(int timeout) {
        m_timeout = timeout;
    }

    @Required
    public void setMongoStatusScript(String mongoStatusScript) {
        m_mongoStatusScript = mongoStatusScript;
    }

    @Required
    public void setMongoAnalyzerScript(String mongoAnalyzerScript) {
        m_mongoAnalyzerScript = mongoAnalyzerScript;
    }
}
