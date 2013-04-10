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

import static org.junit.Assert.assertEquals;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.commons.io.IOUtils;
import org.junit.Before;
import org.junit.Test;
import org.sipfoundry.sipxconfig.commserver.Location;

import com.mongodb.util.JSON;

public class MongoActionModelTest {
    
    static final Location[] LOCATIONS = new Location[] {
        new Location("one"),
        new Location("two"),
        new Location("three"),
        new Location("four"),
        new Location("five")
    };
    
    private Map<String, MongoService> m_mongos;
    private Set<String> m_dbIds;
    
    @SuppressWarnings("unchecked")
    @Before
    public void setUp() throws IOException {
        LOCATIONS[0].setPrimary(true);
        m_mongos = new HashMap<String, MongoService>();
        m_dbIds = new HashSet<String>();
        InputStream in = this.getClass().getResourceAsStream("mongo-status.json");
        String s = IOUtils.toString(in);              
        List<Object> statuses = (List<Object>) JSON.parse(s);
        for (Object status : statuses) {
            MongoService service = new MongoService((Map<String, Object>)status);
            m_mongos.put(service.getServerId(), service);            
            m_dbIds.add(service.getServerId());
        }
    }

    @Test
    public void test() {
        MongoActionModel model = new MongoActionModel();
        model.setMongos(m_mongos);
        assertEquals("[RESTART, STEP_DOWN]",
                model.buildDatabaseServerActions("one:27017", LOCATIONS[0], m_dbIds).toString());
        assertEquals("[RESTART, REMOVE_DATABASE]",
                model.buildDatabaseServerActions("two:27017", LOCATIONS[1], m_dbIds).toString());
        assertEquals("[RESTART, REMOVE_ARBITER]",
                model.buildArbiterServerActions("three:27018", LOCATIONS[2], m_dbIds).toString());
        assertEquals("[RESTART, REMOVE_DATABASE]",
                model.buildDatabaseServerActions("four:27017", LOCATIONS[3], m_dbIds).toString());
        assertEquals("[RESTART, REMOVE_DATABASE]",
                model.buildDatabaseServerActions("five:27017", LOCATIONS[4], m_dbIds).toString());
    }
}
