/**
 *
 *
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
package org.sipfoundry.sipxconfig.site.mongo;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.commons.collections.CollectionUtils;
import org.apache.commons.lang.StringUtils;
import org.apache.tapestry.IAsset;
import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.annotations.Asset;
import org.apache.tapestry.annotations.Bean;
import org.apache.tapestry.annotations.InitialValue;
import org.apache.tapestry.annotations.InjectObject;
import org.apache.tapestry.components.IPrimaryKeyConverter;
import org.apache.tapestry.event.PageBeginRenderListener;
import org.apache.tapestry.event.PageEvent;
import org.sipfoundry.sipxconfig.common.UserException;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.components.PageWithCallback;
import org.sipfoundry.sipxconfig.components.SelectMap;
import org.sipfoundry.sipxconfig.components.SipxValidationDelegate;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.mongo.MongoManager;
import org.sipfoundry.sipxconfig.mongo.MongoMeta;
import org.sipfoundry.sipxconfig.mongo.MongoNode;
import org.sipfoundry.sipxconfig.mongo.MongoSettings;

public abstract class ManageMongo extends PageWithCallback implements PageBeginRenderListener {
    private enum Action {
        ADD_ARBITER, ADD_DATABASE
    }
    public static final String PAGE = "mongo/ManageMongo";

    @Bean
    public abstract SipxValidationDelegate getValidator();

    @InjectObject("spring:featureManager")
    public abstract FeatureManager getFeatureManager();

    @InjectObject("spring:locationsManager")
    public abstract LocationsManager getLocationsManager();

    @InjectObject("spring:mongoManager")
    public abstract MongoManager getMongoManager();

    public abstract MongoMeta getMeta();

    public abstract void setMeta(MongoMeta meta);

    public abstract MongoNode getNode();

    public abstract String getStatus();

    public abstract String getAction();

    public abstract String getMongoAction();

    @Bean
    public abstract SelectMap getSelections();

    @Bean
    public abstract SelectMap getMongoSelections();

    public Collection< ? > getAllSelected() {
        return getSelections().getAllSelected();
    }

    @Asset("/images/server.png")
    public abstract IAsset getServerIcon();

    @Asset("/images/unknown.png")
    public abstract IAsset getUnknownIcon();

    @Asset("/images/error.png")
    public abstract IAsset getErrorIcon();

    @Asset("/images/running.png")
    public abstract IAsset getRunningIcon();

    @Asset("/images/server.png")
    public abstract IAsset getUnconfiguredIcon();

    @Asset("/images/cross.png")
    public abstract IAsset getStoppedIcon();

    @Asset("/images/loading.png")
    public abstract IAsset getLoadingIcon();

    @InitialValue(value = "literal:")
    public abstract String getServerName();

    public abstract void setServerName(String name);

    public abstract String getCurrentServerName();

    public abstract void setNodes(List<MongoNode> status);

    public abstract void setServerNames(Collection<String> names);

    public boolean isServerNameSelected() {
        // effectively clears form every refresh
        return false;
    }

    public void setServerNameSelected(boolean yes) {
        if (yes) {
            setServerName(getCurrentServerName());
        }
    }

    public void onSpecificServerAction(IRequestCycle cycle) {
        Object[] params = cycle.getListenerParameters();
        String serverId = params[0].toString();
        String action = params[1].toString();
        String response = getMongoManager().takeAction(serverId, action);
        String msg;
        if (response == null) {
            msg = "operation working on the background";
        } else {
            msg = action + " on " + serverId + " complete.";
        }
        getValidator().recordSuccess(msg);
        initializePage();
    }

    public String getLocalizedAction() {
        return getAction();
    }

    public String getLocalizedStatus() {
        return getStatus();
    }

    @InitialValue(value = "literal:")
    public abstract String getServerAction();

    public abstract void setServerAction(String action);

    public abstract String getCurrentServerAction();

    public Collection<String> getServerActions() {
        return Arrays.asList(new String[] {Action.ADD_ARBITER.toString(), Action.ADD_DATABASE.toString()});
    }

    public boolean isServerActionSelected() {
        // effectively clears form every refresh
        return false;
    }

    public void setServerActionSelected(boolean yes) {
        if (yes) {
            setServerAction(getCurrentServerAction());
        }
    }

    public IAsset getStatusAsset() {
        for (String s : getNode().getStatus()) {
            if (s.equals("PRIMARY") || s.equals("SECONDARY") || s.equals("ARBITER")) {
                return getRunningIcon();
            }
            if (s.equals("UNAVAILABLE")) {
                return getStoppedIcon();
            }
        }

        return getUnknownIcon();
    }

    @Override
    public void pageBeginRender(PageEvent arg0) {
        if (getMeta() == null) {
            initializePage();
        }
    }

    void initializePage() {
        MongoManager manager = getMongoManager();
        MongoMeta meta = manager.getMeta();
        setMeta(meta);
        List<Location> l = getLocationsManager().getLocationsList();

        // Determine list of only servers that can fit another db component
        @SuppressWarnings("unchecked")
        Collection<String> all = CollectionUtils.collect(l, Location.GET_HOSTNAME);
        Set<String> available = new HashSet<String>(all);
        Set<String> nodes = new HashSet<String>(meta.getServers());
        for (String fqdn : all) {
            if (nodes.contains(fqdn + ':' + MongoSettings.ARBITER_PORT)) {
                if (nodes.contains(fqdn + ':' + MongoSettings.ARBITER_PORT)) {
                    available.remove(fqdn);
                }
            }
        }
        setServerNames(available);
    }

    public void refresh() {
        // nop
    }

    public void takeAction() {
        String serverAction = getServerAction();
        if (StringUtils.isNotBlank(serverAction)) {
            String server = getServerName();
            if (StringUtils.isBlank(server)) {
                getValidator().record(new UserException("&error.selectServer"), getMessages());
                return;
            }
            Action action = Action.valueOf(serverAction);
            MongoNode primary = getMeta().getPrimary();
            if (primary == null) {
                getValidator().record(new UserException("&error.noPrimary"), getMessages());
                return;
            }
            String primaryServer = primary.getId();
            if (action == Action.ADD_ARBITER) {
                getMongoManager().addArbiter(primaryServer, server);
            } else if (action == Action.ADD_DATABASE) {
                getMongoManager().addDatabase(primaryServer, server);
            }

            getValidator().recordSuccess("congrats, you pressed " + serverAction + " for " + server);
        }
    }

    public IPrimaryKeyConverter getConverter() {
        return new IPrimaryKeyConverter() {

            @Override
            public Object getValue(Object arg0) {
                if (arg0 instanceof MongoNode) {
                    return arg0;
                } else if (arg0 instanceof String) {
                    return getMeta().getNode((String) arg0);
                }
                return null;
            }

            @Override
            public Object getPrimaryKey(Object arg0) {
                if (arg0 instanceof MongoNode) {
                    MongoNode mongo = (MongoNode) arg0;
                    return mongo.getId();
                }
                return null;
            }
        };
    }
}
