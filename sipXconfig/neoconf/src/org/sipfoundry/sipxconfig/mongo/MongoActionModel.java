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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import org.apache.commons.collections.CollectionUtils;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.feature.FeatureManager;

public class MongoActionModel {
    private Map<String, Collection<MongoAction>> m_serverActions = new TreeMap<String, Collection<MongoAction>>();
    private Collection<String> m_availableServerIds;
    private Map<String, MongoService> m_mongos;
    private FeatureManager m_featureManager;
    private LocationsManager m_locationsManager;

    @SuppressWarnings("unchecked")
    void init() {
        // Originally thought it would be useful to remove servers where
        // no possible action is valid, but such a small subset, removal
        // would be more disconcerting
        List<Location> all = m_locationsManager.getLocationsList();
        m_availableServerIds = CollectionUtils.collect(all, Location.GET_HOSTNAME);

        Set<String> dbIds = new HashSet<String>();
        for (Location l : m_featureManager.getLocationsForEnabledFeature(MongoManager.FEATURE_ID)) {
            dbIds.add(l.getFqdn() + ':' + MongoSettings.SERVER_PORT);
        }

        Set<String> arbiterIds = new HashSet<String>();
        for (Location l : m_featureManager.getLocationsForEnabledFeature(MongoManager.ARBITER_FEATURE)) {
            arbiterIds.add(l.getFqdn() + ':' + MongoSettings.ARBITER_PORT);
        }

        String dbId;
        Collection<MongoAction> actions;
        for (Location l : all) {
            dbId = l.getFqdn() + ':' + MongoSettings.SERVER_PORT;
            actions = buildDatabaseServerActions(dbId, l, dbIds);
            m_serverActions.put(dbId, actions);

            dbId = l.getFqdn() + ':' + MongoSettings.ARBITER_PORT;
            actions = buildArbiterServerActions(dbId, l, arbiterIds);
            m_serverActions.put(dbId, actions);
        }
    }

    public void setMongos(Map<String, MongoService> mongos) {
        m_mongos = mongos;
    }

    public Collection<String> getAvailableServerIds() {
        return m_availableServerIds;
    }

    public Collection<MongoAction> getAvailableServerActions() {
        return Arrays.asList(MongoAction.ADD_DATABASE, MongoAction.ADD_ARBITER);
    }

    public Collection<MongoAction> getAvailableServerActions(String serverId) {
        return m_serverActions.get(serverId);
    }

    Collection<MongoAction> buildDatabaseServerActions(String dbId, Location l, Set<String> dbIds) {
        List<MongoAction> actions = buildServerActions(dbId, l, dbIds);

        if (dbIds.contains(dbId)) {
            actions.add(MongoAction.RESTART_DATABASE);
            if (!l.isPrimary()) {
                actions.add(MongoAction.REMOVE_DATABASE);
            }
        }

        if (m_mongos.containsKey(dbId)) {
            MongoService service = m_mongos.get(dbId);
            MongoService.State state = service.getState();
            actions.add(MongoAction.FORCE_PRIMARY);
            if (state == MongoService.State.PRIMARY) {
                if (hasMoreThanTwoActiveServers()) {
                    actions.add(MongoAction.STEP_DOWN);
                }
            }
            if (state == MongoService.State.STARTUP1) {
                actions.add(MongoAction.ADD_DATABASE);
            }
            if (state == MongoService.State.MISCONFIGURED || state == MongoService.State.REMOVED) {
                actions.add(MongoAction.CLEAR_LOCAL);
            }
        }

        return actions;
    }

    Collection<MongoAction> buildArbiterServerActions(String dbId, Location l, Set<String> dbIds) {
        List<MongoAction> actions = buildServerActions(dbId, l, dbIds);

        if (dbIds.contains(dbId)) {
            actions.add(MongoAction.RESTART_ARBITER);
            actions.add(MongoAction.REMOVE_ARBITER);
        }
        if (m_mongos.containsKey(dbId)) {
            MongoService service = m_mongos.get(dbId);
            MongoService.State state = service.getState();
            if (state == MongoService.State.STARTUP1) {
                actions.add(MongoAction.ADD_ARBITER);
            }
            if (state == MongoService.State.MISCONFIGURED || state == MongoService.State.REMOVED) {
                actions.add(MongoAction.CLEAR_ARBITER);
            }
        }

        return actions;
    }

    List<MongoAction> buildServerActions(String dbId, Location l, Set<String> dbIds) {
        List<MongoAction> actions = new ArrayList<MongoAction>();
        if (m_mongos.containsKey(dbId)) {
            MongoService service = m_mongos.get(dbId);
            MongoService.State state = service.getState();
            if (state == MongoService.State.NAME_MISMATCH) {
                actions.add(MongoAction.RESET_BAD_HOSTNAMES);
            }
        }

        return actions;
    }

    boolean hasMoreThanTwoActiveServers() {
        boolean hasPrimary = false;
        boolean hasSecondary = false;
        for (MongoService mongo : m_mongos.values()) {
            switch (mongo.getState()) {
            case PRIMARY:
                hasPrimary = true;
                break;
            case SECONDARY:
                hasSecondary = true;
                break;
            default:
                break;
            }
            if (hasSecondary && hasPrimary) {
                return true;
            }
        }
        return false;
    }

    public void setFeatureManager(FeatureManager featureManager) {
        m_featureManager = featureManager;
    }

    public void setLocationsManager(LocationsManager locationsManager) {
        m_locationsManager = locationsManager;
    }
}
