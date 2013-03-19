/**
 *
 *
 * Copyright (c) 2012 eZuce, Inc. All rights reserved.
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
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;
import java.util.List;

import org.apache.commons.collections.CollectionUtils;
import org.apache.commons.io.IOUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.sipfoundry.sipxconfig.cfgmgt.CfengineModuleConfiguration;
import org.sipfoundry.sipxconfig.cfgmgt.ConfigManager;
import org.sipfoundry.sipxconfig.cfgmgt.ConfigProvider;
import org.sipfoundry.sipxconfig.cfgmgt.ConfigRequest;
import org.sipfoundry.sipxconfig.cfgmgt.KeyValueConfiguration;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.feature.FeatureManager;

public class MongoConfig implements ConfigProvider {
    private static final Log LOG = LogFactory.getLog(MongoConfig.class);
    private MongoManager m_mongoManager;
    private MongoReplicaSetManager2 m_mongoReplicateSetManager;
    private boolean m_incomplete;

    @Override
    public void replicate(ConfigManager manager, ConfigRequest request) throws IOException {
        if (!request.applies(MongoManager.FEATURE_ID, LocationsManager.FEATURE, MongoManager.ARBITER_FEATURE)) {
            return;
        }
        FeatureManager fm = manager.getFeatureManager();
        Location[] all = manager.getLocationManager().getLocations();
        MongoSettings settings = m_mongoManager.getSettings();
        List<Location> dbs = manager.getFeatureManager().getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
        List<Location> live = m_mongoReplicateSetManager.getActiveMongoDatabase();
        if (!CollectionUtils.isEqualCollection(dbs, live)) {
            m_incomplete = true;
            LOG.error("mongo client config is incomplete");
        }
        String connStr = getConnectionString(live, settings.getPort());
        String connUrl = getConnectionUrl(live, settings.getPort());
        for (Location location : all) {
            // CLIENT
            File dir = manager.getLocationDataDirectory(location);
            FileWriter client = new FileWriter(new File(dir, "mongo-client.ini"));
            try {
                writeClientConfig(client, connStr, connUrl);
            } finally {
                IOUtils.closeQuietly(client);
            }

            // SERVERS
            boolean mongod = fm.isFeatureEnabled(MongoManager.FEATURE_ID, location);
            boolean arbiter = fm.isFeatureEnabled(MongoManager.ARBITER_FEATURE, location);
            FileWriter server = new FileWriter(new File(dir, "mongodb.cfdat"));
            try {
                writeServerConfig(server, mongod, arbiter);
            } finally {
                IOUtils.closeQuietly(server);
            }
        }
    }

    public void setMongoReplicateSetManager(MongoReplicaSetManager2 mongoReplicateSetManager) {
        m_mongoReplicateSetManager = mongoReplicateSetManager;
    }

    public boolean isIncomplete() {
        return m_incomplete;
    }

    void writeServerConfig(Writer w, boolean mongod, boolean arbiter) throws IOException {
        String bindToAll = "0.0.0.0";
        CfengineModuleConfiguration config = new CfengineModuleConfiguration(w);
        config.writeClass("mongod", mongod);
        config.write("mongoBindIp", bindToAll);
        config.write("mongoPort", MongoSettings.SERVER_PORT);
        config.writeClass("mongod_arbiter", arbiter);
        config.write("mongoArbiterBindIp", bindToAll);
        config.write("mongoArbiterPort", MongoSettings.ARBITER_PORT);
    }

    void writeClientConfig(Writer w, String connStr, String connUrl) throws IOException {
        KeyValueConfiguration config = KeyValueConfiguration.equalsSeparated(w);
        config.write("connectionUrl", connUrl);
        config.write("connectionString", connStr);
    }

    String getConnectionString(List<Location> servers, int port) {
        StringBuilder r = new StringBuilder("sipxecs/");
        for (int i = 0; i < servers.size(); i++) {
            Location server = servers.get(i);
            if (i > 0) {
                r.append(',');
            }
            r.append(server.getFqdn() + ':' + port);
        }
        return r.toString();
    }

    String getConnectionUrl(List<Location> servers, int port) {
        StringBuilder r = new StringBuilder("mongodb://");
        for (int i = 0; i < servers.size(); i++) {
            Location server = servers.get(i);
            if (i > 0) {
                r.append(',');
            }
            r.append(server.getFqdn() + ':' + port);
        }
        r.append("/?slaveOk=true");
        return r.toString();
    }

    public void setMongoManager(MongoManager mongoManager) {
        m_mongoManager = mongoManager;
    }
}
