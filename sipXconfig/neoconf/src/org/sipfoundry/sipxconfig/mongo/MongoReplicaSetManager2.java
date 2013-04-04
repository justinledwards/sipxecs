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

import java.io.IOException;
import java.io.InputStream;
import java.net.ConnectException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.sipfoundry.sipxconfig.cfgmgt.ConfigManager;
import org.sipfoundry.sipxconfig.cfgmgt.RunRequest;
import org.sipfoundry.sipxconfig.common.UserException;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.feature.LocationFeature;
import org.springframework.beans.BeansException;
import org.springframework.beans.factory.BeanFactory;
import org.springframework.beans.factory.BeanFactoryAware;

public class MongoReplicaSetManager2 implements BeanFactoryAware {
    private static final Log LOG = LogFactory.getLog(MongoReplicaSetManager2.class);
    private FeatureManager m_featureManager;
    private LocationsManager m_locationsManager;
    private ConfigManager m_configManager;
    private String m_replicationAdminScript;
    private BeanFactory m_beanFactory;
    private Thread m_lastJob;
    private UserException m_lastError;
    private Map<MongoAction, MongoOperation> m_operations;

    public FeatureManager getFeatureManager() {
        return m_featureManager;
    }

    public LocationsManager getLocationsManager() {
        return m_locationsManager;
    }

    public ConfigManager getConfigManager() {
        return m_configManager;
    }

    public MongoAdmin getMongoAdmin() {
        return (MongoAdmin) m_beanFactory.getBean("mongoAdmin");
    }

    public MongoActionModel getActionModel(Map<String, MongoService> services) {
        MongoActionModel model = (MongoActionModel) m_beanFactory.getBean("mongoActionModel");
        model.setMongos(services);
        model.init();
        return model;
    }

    public List<Location> getActiveMongoDatabase() {
        List<Location> dbs = m_featureManager.getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
        List<Location> live = new ArrayList<Location>();
        Map<String, MongoService> status = getMongoServices();
        for (Location l : dbs) {
            String key = l.getFqdn() + ':' + MongoSettings.SERVER_PORT;
            if (status.containsKey(key)) {
                live.add(l);
            }
        }
        return live;
    }

    public Map<String, MongoService> getMongoServices() {
        final Map<String, MongoService> nodes = new TreeMap<String, MongoService>();
        final AbstractMongoOperation.CommandReader reader = new AbstractMongoOperation.CommandReader() {
            @Override
            public void read(AbstractMongoOperation op, InputStream in) throws Exception {
                List<Object> jsonNodes = op.readJson(in);
                for (Object o : jsonNodes) {
                    Map<String, ? > map = (Map<String, ? >) o;
                    MongoService node = new MongoService(map);
                    nodes.put(node.getServerId(), node);
                }
            }
        };
        final MongoOperation status = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                List<String> cmd = getReplicationCommand(request.getManager(), "--status");
                run(cmd, reader);
                return false;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
            }
        };

        MongoOperationRequest request = new MongoOperationRequest(this, null, null, null);
        status.runNow(request);
        return nodes;
    }

    void enableDatabase(MongoAction action, String serverId) {
        Location l = m_locationsManager.getLocationByFqdn(serverId);
        LocationFeature f;
        boolean enable;
        switch (action) {
        case REMOVE_ARBITER:
            f = MongoManager.ARBITER_FEATURE;
            enable = false;
            break;
        case ADD_ARBITER:
            f = MongoManager.ARBITER_FEATURE;
            enable = true;
            break;
        case ADD_DATABASE:
            f = MongoManager.FEATURE_ID;
            enable = true;
            break;
        case REMOVE_DATABASE:
            List<Location> dbs = m_featureManager.getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
            if (dbs.size() <= 1) {
                throw new UserException("&error.noMongos");
            }
            f = MongoManager.FEATURE_ID;
            enable = false;
            break;
        default:
            throw new IllegalStateException();
        }
        m_featureManager.enableLocationFeature(f, l, enable);
    }

    void activateDatabase(AbstractMongoOperation operation, MongoAction action, String serverId, String fqdn) {
        String arbiterAddress = fqdn + ':' + MongoSettings.ARBITER_PORT;
        String dbAddress = fqdn + ':' + MongoSettings.SERVER_PORT;
        List<String> cmd;
        String remove = "--remove";
        switch (action) {
        case ADD_DATABASE:
            if (!waitUntilConnectionAvailable(dbAddress)) {
                throw new UserException("Database not available " + dbAddress);
            }
            cmd = operation.getReplicationCommand(this, "--add", dbAddress);
            break;

        case ADD_ARBITER:
            if (!waitUntilConnectionAvailable(arbiterAddress)) {
                throw new UserException("Arbiter not available " + dbAddress);
            }
            cmd = operation.getReplicationCommand(this, "--addArbiter", arbiterAddress);
            break;

        case REMOVE_DATABASE:
            cmd = operation.getReplicationCommand(this, remove, dbAddress);
            break;

        case REMOVE_ARBITER:
            cmd = operation.getReplicationCommand(this, remove, arbiterAddress);
            break;

        default:
            throw new IllegalStateException();
        }
        operation.run(cmd, null);
    }

    void runRequest(MongoAction action, String fqdn) {
        Location l = m_locationsManager.getLocationByFqdn(fqdn);
        RunRequest rr = new RunRequest("MONGO " + action, Collections.singleton(l));
        // commands are implemented in file sipXmongo/etc/mongodb_actions.cf
        rr.setBundles("mongodb_actions");
        rr.setDefines(action.toString());
        getConfigManager().run(rr);
    }

    public Map<MongoAction, MongoOperation> getOperations() {
        if (m_operations != null) {
            return m_operations;
        }
        Map<MongoAction, MongoOperation> operations = new HashMap<MongoAction, MongoOperation>();
        MongoOperation addOrRemove = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                enableDatabase(request.getAction(), request.getServerId());
                return true;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
                String fqdn = request.getServerId();
                activateDatabase(this, request.getAction(), request.getServerId(), fqdn);
            }
        };

        operations.put(MongoAction.ADD_DATABASE, addOrRemove);
        operations.put(MongoAction.ADD_ARBITER, addOrRemove);
        operations.put(MongoAction.REMOVE_ARBITER, addOrRemove);
        operations.put(MongoAction.REMOVE_DATABASE, addOrRemove);

        MongoOperation runRequest = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                String fqdn = MongoService.label(request.getServerId());
                runRequest(request.getAction(), fqdn);
                return false;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
            }
        };

        operations.put(MongoAction.RESTART_DATABASE, runRequest);
        operations.put(MongoAction.RESTART_ARBITER, runRequest);
        operations.put(MongoAction.FORCE_PRIMARY, runRequest);
        operations.put(MongoAction.CLEAR_LOCAL, runRequest);
        operations.put(MongoAction.CLEAR_ARBITER, runRequest);

        MongoOperation simpleCommand = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                List<String> cmd = null;
                switch (request.getAction()) {
                case RESET_BAD_HOSTNAMES:
                    cmd = getReplicationCommand(MongoReplicaSetManager2.this, "--name");
                    break;
                default:
                    throw new IllegalStateException();
                }
                run(cmd, null);
                return false;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
            }
        };

        // reset bad hostname can run on any server, but might as well
        // run command on server that has bad hostname.
        operations.put(MongoAction.RESET_BAD_HOSTNAMES, simpleCommand);

        m_operations = operations;
        return operations;
    }

    public boolean waitUntilConnectionAvailable(String hostPort) {
        String[] split = StringUtils.split(hostPort, ':');
        InetSocketAddress address = new InetSocketAddress(split[0], Integer.valueOf(split[1]));
        for (int i = 0; i < 10; i++) {
            try {
                Socket test = new Socket();
                test.connect(address, 10000);
                if (i != 0) {
                    // if this failed previously, then there's a chance
                    // system just came up. Give mongo time to initialize
                    try {
                        Thread.sleep(10000);
                    } catch (InterruptedException e) {
                        LOG.error("Interrupted trying to wait for additional time for mongo init");
                    }
                }
                return true;
            } catch (ConnectException e) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e1) {
                    LOG.error("Interrupted testing connection", e1);
                    return false;
                }
            } catch (IOException e) {
                LOG.error("IO Error testing connection", e);
                return false;
            }
        }

        return false;
    }

    public boolean isOperationIsProgress() {
        return m_lastJob != null && m_lastJob.isAlive();
    }

    public synchronized void cancelLastJob() {
        m_lastError = null;
        if (m_lastJob != null) {
            if (m_lastJob.isAlive()) {
                m_lastJob.interrupt();
            }
            m_lastJob = null;
        }
    }

    public synchronized void takeAction(MongoAction action, final String serverId, final String... params) {
        m_lastError = null;
        if (m_lastJob != null) {
            if (m_lastJob.isAlive()) {
                throw new UserException("Operation still in progress.");
            }
        }

        // reusing operations relies on operations having no state
        final MongoOperation job = getOperations().get(action);

        final MongoOperationRequest request = new MongoOperationRequest(this, action, serverId, params);
        if (job.runNow(request)) {
            m_lastJob = new Thread(action.toString()) {
                public void run() {
                    try {
                        job.runLater(request);
                    } catch (UserException e) {
                        m_lastError = e;
                    }
                }
            };
            m_lastJob.start();
            try {
                m_lastJob.join(15000);
            } catch (InterruptedException noProblemWeWillCheckBackLater) {
                LOG.info("Allowing mongo configuration task to run in background");
            }
        }
    }

    public UserException getLastError() {
        return m_lastError;
    }

    public String getReplicationAdminScript() {
        return m_replicationAdminScript;
    }

    public void setFeatureManager(FeatureManager featureManager) {
        m_featureManager = featureManager;
    }

    public void setReplicationAdminScript(String replicationAdminScript) {
        m_replicationAdminScript = replicationAdminScript;
    }

    public void setLocationsManager(LocationsManager locationsManager) {
        m_locationsManager = locationsManager;
    }

    public void setConfigManager(ConfigManager configManager) {
        m_configManager = configManager;
    }

    @Override
    public void setBeanFactory(BeanFactory arg0) throws BeansException {
        m_beanFactory = arg0;
    }
}
