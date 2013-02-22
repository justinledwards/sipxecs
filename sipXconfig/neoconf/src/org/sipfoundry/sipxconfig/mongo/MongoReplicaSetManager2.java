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

    public MongoActionModel getActionModel(Map<String, MongoService> services) {
        MongoActionModel model = (MongoActionModel) m_beanFactory.getBean("mongoActionModel");
        model.setMongos(services);
        model.init();
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

    void enableDatabase(MongoAction action, String serverId) {
        Location l = m_locationsManager.getLocationByFqdn(serverId);
        LocationFeature f;
        boolean enable;
        switch (action) {
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
            throw new IllegalStateException();
        }
        m_featureManager.enableLocationFeature(f, l, enable);
    }

    void activeDatabase(AbstractMongoOperation operation, MongoAction action, String serverId, String fqdn) {
        Location l = m_locationsManager.getLocationByFqdn(serverId);
        String arbiterAddress = fqdn + ':' + MongoSettings.ARBITER_PORT;
        String dbAddress = fqdn + ':' + MongoSettings.SERVER_PORT;
        List<String> cmd;
        LocationFeature f;
        boolean enable;
        String remove = "--remove";
        switch (action) {
        case ADD_DATABASE:
            // wait until STARTED/STOPPED
            waitUntilDatabaseAvailable(dbAddress);
        case FINISH_INCOMPLETE_ADD_DATABASE:
            f = MongoManager.ACTIVE_DATABASE;
            enable = true;
            cmd = operation.getReplicationCommand(this, "--add", dbAddress);
            break;

        case ADD_ARBITER:
            // wait until STARTED/STOPPED
            waitUntilDatabaseAvailable(arbiterAddress);
        case FINISH_INCOMPLETE_ADD_ARBITER:
            f = MongoManager.ACTIVE_ARBITER;
            enable = true;
            cmd = operation.getReplicationCommand(this, "--addArbiter", arbiterAddress);
            break;

        case REMOVE_DATABASE:
            f = MongoManager.FEATURE_ID;
            enable = false;
            cmd = operation.getReplicationCommand(this, remove, dbAddress);
            break;

        case REMOVE_ARBITER:
            f = MongoManager.ARBITER_FEATURE;
            enable = false;
            cmd = operation.getReplicationCommand(this, remove, arbiterAddress);
            break;

        default:
            throw new IllegalStateException();
        }
        operation.run(cmd, null);
        // roll out config
        m_featureManager.enableLocationFeature(f, l, enable);
    }

    void runRequest(MongoAction action, String fqdn) {
        Location l = m_locationsManager.getLocationByFqdn(fqdn);
        RunRequest rr = new RunRequest("MONGO " + action, Collections.singleton(l));
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
                activeDatabase(this, request.getAction(), request.getServerId(), fqdn);
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

        MongoOperation finshAdd = new AbstractMongoOperation() {
            @Override
            public boolean runNow(MongoOperationRequest request) {
                String fqdn = MongoService.label(request.getServerId());
                activeDatabase(this, request.getAction(), request.getServerId(), fqdn);
                return false;
            }

            @Override
            public void runLater(MongoOperationRequest request) {
            }
        };

        operations.put(MongoAction.FINISH_INCOMPLETE_ADD_ARBITER, finshAdd);
        operations.put(MongoAction.FINISH_INCOMPLETE_ADD_DATABASE, finshAdd);

        m_operations = operations;
        return operations;
    }

    public void waitUntilDatabaseAvailable(String address) {
        return;
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
