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

import static org.easymock.EasyMock.createMock;
import static org.easymock.EasyMock.expectLastCall;
import static org.easymock.EasyMock.replay;
import static org.easymock.EasyMock.verify;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.io.File;
import java.io.IOException;
import java.net.ConnectException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Collections;
import java.util.Map;

import org.apache.commons.lang.StringUtils;
import org.junit.Before;
import org.junit.Test;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.test.TestHelper;

public class MongoReplicaSetManager2Test {
    private MongoReplicaSetManager2 m_manager;
    
    @Before
    public void setup() {
        m_manager = new MongoReplicaSetManager2();        
        File script = TestHelper.getResourceAsFile(getClass(), "mongo-replication-admin");
        m_manager.setReplicationAdminScript(script.getAbsolutePath());
    }
    
    @Test
    public void testConnect() throws NumberFormatException, UnknownHostException, IOException {
        String hostPort = "localhost:27019";
        String[] split = StringUtils.split(hostPort, ':');
        InetSocketAddress address = new InetSocketAddress(split[0], Integer.valueOf(split[1]));
        boolean connected = false;
        for (int i = 0; i < 10; i++) {
            try {
                Socket test = new Socket();
                test.connect(address, 10000);
                connected = true;
                break;
            } catch (ConnectException e) {
                System.out.print("here");
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e1) {
                    break;
                }
            }
        }
        System.out.print(connected ? "connected" : "no connection");
    }

    @Test
    public void testNodes() throws IOException, InterruptedException {
        FeatureManager fm = createMock(FeatureManager.class);
        fm.getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
        expectLastCall().andReturn(Collections.emptyList());
        fm.getLocationsForEnabledFeature(MongoManager.ARBITER_FEATURE);
        expectLastCall().andReturn(Collections.emptyList());
        replay(fm);
        m_manager.setFeatureManager(fm);
        Map<String, MongoService> nodes = m_manager.getMongoServices();
        assertNotNull(nodes);
        assertEquals(2 ,nodes.size());
        Map<String, ? > status = nodes.get("swift.hubler.us:27018").getStatusMember();
        assertNotNull(status);   
        MongoService[] actual = nodes.values().toArray(new MongoService[0]);
        assertEquals(actual[0].getStatusValue("stateStr"), "PRIMARY");
        assertEquals(actual[1].getStatusValue("stateStr"), "ARBITER");
        verify(fm);
    }
    
}
