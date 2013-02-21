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

import java.io.InputStream;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

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
    private FeatureManager m_featureManager;
    private LocationsManager m_locationsManager;
    private ConfigManager m_configManager;
    private String m_replicationAdminScript;
    private BeanFactory m_beanFactory;
    private Thread m_lastJob;
    private Map<MongoAction, MongoOperation> m_operations = new HashMap<MongoAction, MongoOperation>();

    public FeatureManager getFeatureManager() {
        return m_featureManager;
    }

    public LocationsManager getLocationsManager() {
        return m_locationsManager;
    }

    public ConfigManager getConfigManager() {
        return m_configManager;
    }

    public MongoActionModel getActionModel(Map<String, MongoService> services) {
        MongoActionModel model = (MongoActionModel) m_beanFactory.getBean("mongoActionModel");
        model.setMongos(services);
        return model;
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

    public void buildOperations() {
        Map<MongoAction, MongoOperation> operations = new HashMap<MongoAction, MongoOperation>();
        MongoOperation addOrRemove = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                Location l = m_locationsManager.getLocationByFqdn(request.getServerId());
                LocationFeature f;
                boolean enable;
                switch (request.getAction()) {
                case REMOVE_ARBITER:
                    f = MongoManager.ACTIVE_ARBITER;
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
                    f = MongoManager.ACTIVE_DATABASE;
                    enable = false;
                    break;
                default:
                    throw new RuntimeException("Unrecognized action " + request.getAction());
                }
                request.getManager().getFeatureManager().enableLocationFeature(f, l, enable);
                return true;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
                // wait until STARTED/STOPPED
                // add/remove to replset
                // roll out config
                Location l = m_locationsManager.getLocationByFqdn(request.getServerId());
                String fqdn = request.getServerId();
                String arbiterAddress = fqdn + ':' + MongoSettings.ARBITER_PORT;
                String dbAddress = fqdn + ':' + MongoSettings.SERVER_PORT;
                List<String> cmd;
                LocationFeature f;
                boolean enable;
                switch (request.getAction()) {
                case ADD_DATABASE:
                    waitUntilDatabaseAvailable(dbAddress);
                    f = MongoManager.ACTIVE_DATABASE;
                    enable = true;
                    cmd = getReplicationCommand(request.getManager(), "--add", dbAddress);
                    break;
                case ADD_ARBITER:
                    waitUntilDatabaseAvailable(arbiterAddress);
                    f = MongoManager.ACTIVE_ARBITER;
                    enable = true;
                    cmd = getReplicationCommand(request.getManager(), "--addArbiter", arbiterAddress);
                    break;
                case REMOVE_DATABASE:
                    f = MongoManager.FEATURE_ID;
                    enable = false;
                    cmd = getReplicationCommand(request.getManager(), "--remove", dbAddress);
                    break;
                case REMOVE_ARBITER:
                    f = MongoManager.ARBITER_FEATURE;
                    enable = false;
                    cmd = getReplicationCommand(request.getManager(), "--removeArbiter", arbiterAddress);
                    break;
                default:
                    throw new RuntimeException("Unrecognized action 2 " + request.getAction());
                }
                run(cmd, null);
                m_featureManager.enableLocationFeature(f, l, enable);
            }
        };

        operations.put(MongoAction.ADD_DATABASE, addOrRemove);
        operations.put(MongoAction.ADD_ARBITER, addOrRemove);
        operations.put(MongoAction.REMOVE_ARBITER, addOrRemove);
        operations.put(MongoAction.REMOVE_DATABASE, addOrRemove);

        MongoOperation simpleCommand = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                String fqdn = MongoService.label(request.getServerId());
                Location l = m_locationsManager.getLocationByFqdn(fqdn);
                RunRequest rr = new RunRequest("MONGO " + request.getAction(), Collections.singleton(l));
                rr.setBundles("mongodb_actions");
                rr.setDefines(request.getAction().toString());
                getConfigManager().run(rr);
                return false;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
            }
        };

        operations.put(MongoAction.RESTART_DATABASE, simpleCommand);
        operations.put(MongoAction.RESTART_ARBITER, simpleCommand);
        operations.put(MongoAction.FORCE_PRIMARY, simpleCommand);
        operations.put(MongoAction.CLEAR_LOCAL, simpleCommand);
    }

    public void waitUntilDatabaseAvailable(String address) {
        return;
    }

    public synchronized void cancelLastJob() {
        if (m_lastJob != null) {
            if (m_lastJob.isAlive()) {
                m_lastJob.interrupt();
            }
            m_lastJob = null;
        }
    }

    public synchronized void takeAction(MongoAction action, final String serverId, final String... params) {
        if (m_lastJob != null) {
            if (m_lastJob.isAlive()) {
                throw new UserException("Operation still in progress.");
            }
        }

        // clone in case there is state, but not strictly nec. from current implementation
        final MongoOperation job = m_operations.get(action).clone();

        final MongoOperationRequest request = new MongoOperationRequest(this, action, serverId, params);
        if (job.runNow(request)) {
            m_lastJob = new Thread() {
                public void run() {
                    job.runLater(request);
                }
            };
            m_lastJob.start();
        }
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
