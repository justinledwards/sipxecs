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

import java.util.ArrayList;
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
import org.apache.tapestry.annotations.InjectObject;
import org.apache.tapestry.components.IPrimaryKeyConverter;
import org.apache.tapestry.event.PageBeginRenderListener;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.form.IPropertySelectionModel;
import org.apache.tapestry.form.StringPropertySelectionModel;
import org.sipfoundry.sipxconfig.common.UserException;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.components.ExtraOptionModelDecorator;
import org.sipfoundry.sipxconfig.components.PageWithCallback;
import org.sipfoundry.sipxconfig.components.SipxValidationDelegate;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.mongo.MongoManager;
import org.sipfoundry.sipxconfig.mongo.MongoMeta;
import org.sipfoundry.sipxconfig.mongo.MongoNode;
import org.sipfoundry.sipxconfig.mongo.MongoSettings;

public abstract class ManageMongo extends PageWithCallback implements PageBeginRenderListener {
    public static final String PAGE = "mongo/ManageMongo";

    private static final String DELETE = "_DELETE";

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

    public abstract String getAddDb();

    public abstract void setAddDb(String server);

    public abstract String getAddArbiter();

    public abstract void setAddArbiter(String server);

    public abstract void setNodes(List<MongoNode> status);

    public abstract IPropertySelectionModel getAddDbModel();

    public abstract void setAddDbModel(IPropertySelectionModel model);

    public abstract IPropertySelectionModel getAddArbiterModel();

    public abstract void setAddArbiterModel(IPropertySelectionModel model);

    public abstract String getAvailableAction();

    public abstract void setAvailableAction(String action);

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

    public IPropertySelectionModel getAvailableActionModel() {
        Collection<String> raw = getMeta().getAvailableActions(getNode().getHostPort());
        List<String> actions = new ArrayList<String>(raw);
        actions.add(DELETE);
        IPropertySelectionModel m = new StringPropertySelectionModel(actions.toArray(new String[0]));
        ExtraOptionModelDecorator e = new ExtraOptionModelDecorator();
        e.setExtraLabel("actions...");
        e.setExtraOption(null);
        e.setModel(m);
        return e;
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
        List<String> addArbiters = new ArrayList<String>();
        List<String> addDatabases = new ArrayList<String>();
        Set<String> nodes = new HashSet<String>(meta.getServers());
        for (String fqdn : all) {
            if (!nodes.contains(fqdn + ':' + MongoSettings.ARBITER_PORT)) {
                addArbiters.add(fqdn);
            }
            if (!nodes.contains(fqdn + ':' + MongoSettings.SERVER_PORT)) {
                addDatabases.add(fqdn);
            }
        }

        String[] arbs = addArbiters.toArray(new String[0]);
        ExtraOptionModelDecorator arbM = new ExtraOptionModelDecorator();
        arbM.setModel(new StringPropertySelectionModel(arbs));
        arbM.setExtraLabel("Add arbiter...");
        arbM.setExtraOption(null);
        setAddArbiterModel(arbM);

        String[] dbs = addDatabases.toArray(new String[0]);
        ExtraOptionModelDecorator dbM = new ExtraOptionModelDecorator();
        dbM.setModel(new StringPropertySelectionModel(dbs));
        dbM.setExtraLabel("Add database...");
        dbM.setExtraOption(null);
        setAddDbModel(dbM);

        // clear these form values on page reload
        setAvailableAction(null);
        setAddArbiter(null);
        setAddDb(null);
    }

    public void refresh() {
        // nop
    }

    public void takeAction() {
        String addDb = getAddDb();
        String addArbiter = getAddArbiter();
        if (StringUtils.isNotBlank(addDb) || StringUtils.isNotBlank(addArbiter)) {
            MongoNode primary = requirePrimary();
            if (primary != null) {
                if (StringUtils.isNotBlank(addDb)) {
                    String hostPort = MongoNode.databaseHostPort(addDb);
                    getMongoManager().addDatabase(primary.getHostPort(), hostPort);
                } else if (StringUtils.isNotBlank(addArbiter)) {
                    String hostPort = MongoNode.arbiterHostPort(addArbiter);
                    getMongoManager().addArbiter(primary.getHostPort(), hostPort);
                }
                getValidator().recordSuccess("Service is being added.");
            }
        }

        String action = getAvailableAction();
        if (StringUtils.isNotBlank(action)) {
            MongoNode node = getNode();
            String fqdn = getNode().getFqdn();
            String result = null;
            if (action == DELETE) {
                MongoNode primary = requirePrimary();
                if (primary != null) {
                    if (node.isArbiter()) {
                        getMongoManager().removeArbiter(primary.getHostPort(), getNode().getHostPort());
                    } else {
                        getMongoManager().removeDatabase(primary.getHostPort(), getNode().getHostPort());
                    }
                }
            } else {
                result = getMongoManager().takeAction(fqdn, action);
            }
            if (result == null) {
                getValidator().recordSuccess("Operation performed.");
            } else {
                getValidator().recordSuccess("Operation is running in background.");
            }
        }

        // might need this.
        // initializePage();
    }

    MongoNode requirePrimary() {
        MongoNode primary = getMeta().getPrimary();
        if (primary == null) {
            getValidator().record(new UserException("&error.noPrimary"), getMessages());
        }
        return primary;
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
                    return mongo.getHostPort();
                }
                return null;
            }
        };
    }
}
