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


import java.util.Collection;
import java.util.Collections;
import java.util.Map;

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
import org.sipfoundry.sipxconfig.commserver.LocationsManager;
import org.sipfoundry.sipxconfig.components.PageWithCallback;
import org.sipfoundry.sipxconfig.components.SelectMap;
import org.sipfoundry.sipxconfig.components.SipxValidationDelegate;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.mongo.MongoAction;
import org.sipfoundry.sipxconfig.mongo.MongoActionModel;
import org.sipfoundry.sipxconfig.mongo.MongoManager;
import org.sipfoundry.sipxconfig.mongo.MongoReplicaSetManager;
import org.sipfoundry.sipxconfig.mongo.MongoReplicaSetManager2;
import org.sipfoundry.sipxconfig.mongo.MongoService;

public abstract class ManageMongo extends PageWithCallback implements PageBeginRenderListener {
    public static final String PAGE = "mongo/ManageMongo";

    @Bean
    public abstract SipxValidationDelegate getValidator();

    @InjectObject("spring:featureManager")
    public abstract FeatureManager getFeatureManager();

    @InjectObject("spring:locationsManager")
    public abstract LocationsManager getLocationsManager();

    @InjectObject("spring:mongoManager")
    public abstract MongoManager getMongoManager();

    @InjectObject("spring:mongoReplicaSetManager")
    public abstract MongoReplicaSetManager getMongoReplicaSetManager();

    @InjectObject("spring:mongoReplicaSetManager2")
    public abstract MongoReplicaSetManager2 getMongoReplicaSetManager2();

    public abstract void setMongos(Map<String, MongoService> mongos);

    public abstract Map<String, MongoService> getMongos();

    public abstract MongoService getMongo();

    public abstract MongoActionModel getActionModel();

    public abstract void setActionModel(MongoActionModel model);

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

    public Collection<String> getServerNames() {
        return getActionModel().getAvailableServerIds();
    }

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
        MongoAction action = MongoAction.valueOf(params[0].toString());
        String serverId = params[1].toString();
        getMongoReplicaSetManager2().takeAction(action, serverId);
        getValidator().recordSuccess(action + " on " + serverId + " complete.");
        initializePage();
    }

    @InitialValue(value = "literal:")
    public abstract String getServerAction();

    public abstract void setServerAction(String action);

    public abstract String getCurrentServerAction();

    public Collection<MongoAction> getServerActions() {
        return getActionModel().getAvailableServerActions();
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
        switch (getMongo().getState()) {
        case PRIMARY:
        case ARBITER:
            return getRunningIcon();
        case UNKNOWN:
            return getUnknownIcon();
        case STARTUP1:
            return getUnconfiguredIcon();
        case STARTUP2:
            return getLoadingIcon();
        case UNAVAILABLE:
            return getStoppedIcon();
        default:
            return getErrorIcon();
        }
    }

    @InitialValue(value = "literal:")
    public abstract String getSpecificServerAction();

    public abstract void setSpecificServerAction(String action);

    public abstract String getCurrentSpecificServerAction();

    public Collection<MongoAction> getSpecificServerActions() {
        MongoService service = getMongo();
        Collection<MongoAction> none = Collections.emptyList();
        return service == null ? none : getActionModel().getAvailableServerActions(service.getServerId());
    }

    public boolean isSpecificServerActionSelected() {
        // effectively clears form every refresh
        return false;
    }

    public void setSpecificServerActionSelected(boolean yes) {
        if (yes) {
            setSpecificServerAction(getCurrentSpecificServerAction());
        }
    }

    @Override
    public void pageBeginRender(PageEvent arg0) {
        if (getMongos() == null) {
            initializePage();
        }
    }

    void initializePage() {
        Map<String, MongoService> mongos = getMongos();
        mongos = getMongoReplicaSetManager2().getMongoServices();
        setMongos(mongos);
        MongoActionModel actionModel = getMongoReplicaSetManager2().getActionModel(mongos);
        setActionModel(actionModel);

        UserException lastErr = getMongoReplicaSetManager2().getLastError();
        if (lastErr != null) {
            getValidator().record(lastErr, getMessages());
        }
    }

    public void refresh() {
        // nop
    }

    public void takeAction() {
        String serverAction = getServerAction();
        String specificServerAction = getSpecificServerAction();
        if (StringUtils.isNotBlank(serverAction)) {
            String server = getServerName();
            if (StringUtils.isBlank(server)) {
                getValidator().record(new UserException("&error.selectServer"), getMessages());
            }
            MongoAction action = MongoAction.valueOf(serverAction);
            getMongoReplicaSetManager2().takeAction(action, server);
            getValidator().recordSuccess("congrats, you pressed " + serverAction + " for " + server);
        } else if (StringUtils.isNotBlank(specificServerAction)) {
            String specificServer = getCurrentServerName();
            MongoAction action = MongoAction.valueOf(specificServerAction);
            getMongoReplicaSetManager2().takeAction(action, specificServer);
            getValidator().recordSuccess(
                    "congrats, you pressed specific action " + specificServerAction + " for server "
                            + specificServer);
        }
    }

    public void addInReplicaSet(String name) {
        try {
            getMongoReplicaSetManager().addInReplicaSet(name);
        } catch (Exception ex) {
            getValidator().record(new UserException(ex.getMessage()), getMessages());
        }
    }

    public void removeFromReplicaSet(String name) {
        try {
            getMongoReplicaSetManager().removeFromReplicaSet(name);
        } catch (Exception ex) {
            getValidator().record(new UserException(ex.getMessage()), getMessages());
        }
    }

    public void stepDown() {
        try {
            getMongoReplicaSetManager().stepDown();
        } catch (Exception ex) {
            getValidator().record(new UserException(ex.getMessage()), getMessages());
        }
    }

    public void forceReconfig() {
        try {
            getMongoReplicaSetManager().forceReconfig();
        } catch (Exception ex) {
            getValidator().record(new UserException(ex.getMessage()), getMessages());
        }
    }

    public IPrimaryKeyConverter getConverter() {
        return new IPrimaryKeyConverter() {

            @Override
            public Object getValue(Object arg0) {
                if (arg0 instanceof MongoService) {
                    return arg0;
                } else if (arg0 instanceof String) {
                    return getMongos().get((String) arg0);
                }
                return null;
            }

            @Override
            public Object getPrimaryKey(Object arg0) {
                if (arg0 instanceof MongoService) {
                    MongoService mongo = (MongoService) arg0;
                    return mongo.getServerId();
                }
                return null;
            }
        };
    }

}
