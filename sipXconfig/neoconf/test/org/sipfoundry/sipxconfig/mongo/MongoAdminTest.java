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
import static org.junit.Assert.assertNotNull;

import java.io.IOException;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import org.apache.commons.io.IOUtils;
import org.junit.Test;
import org.sipfoundry.sipxconfig.commserver.Location;

public class MongoAdminTest {
    
    @Test
    public void getNodesByServer() throws IOException {
        MongoAdmin admin = new MongoAdmin();
        String token = IOUtils.toString(getClass().getResourceAsStream("three-node-healthy.status.json"));        
        Map<String, MongoNode> nodes = admin.getNodesByServer(token);
        assertEquals(3, nodes.size());
        assertNotNull(nodes.get("swift.hubler.us:27017"));
    }
    
    @Test
    public void getServerList() throws IOException {
        Location s1 = new Location("one");
        Location s2 = new Location("two");
        Location s3 = new Location("three");
        Location s4 = new Location("four");
        Arrays.asList(s1, s2);
        Arrays.asList(s3, s4);
        MongoAdmin admin = new MongoAdmin();
        StringWriter actual = new StringWriter();
        admin.serverList(actual, Arrays.asList(s1, s2), Arrays.asList(s3, s4));
        assertEquals("{ \"servers\" : [ \"one:27017\" , \"two:27017\"] , \"arbiters\" : "
                + "[ \"one:27018\" , \"two:27018\"] , \"replSet\" : \"sipxecs\"}", actual.toString());
    }
    
    // not run as command not usable in unit test env.
    // @Test
    public void getStatusToken() {
        MongoAdmin admin = new MongoAdmin();
        String token = admin.getStatusToken();
        assertEquals("{\"swift.hubler.us:27018\": [], \"CLUSTER\": [], "
                + "\"swift.hubler.us:27017\": [\"PRIMARY\"]}\n", token);
    }
    
    // not run as command not usable in unit test env.
    // @Test
    public void getAnalysisToken() throws IOException {
        MongoAdmin admin = new MongoAdmin();
        String token = IOUtils.toString(getClass().getResourceAsStream("three-node-healthy.status.json"));        
        String analysisToken = admin.getAnalysisToken(token);
        assertEquals("{}\n", analysisToken);
    }

    @Test
    public void getAnalyis() throws IOException {
        MongoAdmin admin = new MongoAdmin();
        String analToken = IOUtils.toString(getClass().getResourceAsStream("three-node-missing-arbiter-and-database.analysis.json"));
        
        Map<String, Map<String, List<String>>> actual = admin.getAnalysis(analToken);
        List<String> req = actual.get("required").get("swift.hubler.us:27017");
        assertEquals("[ \"ADD swift.hubler.us:27018\" , \"ADD swift.hubler.us:27019\"]", req.toString());
    }

    @Test
    public void getStatus() throws IOException {
        MongoAdmin admin = new MongoAdmin();
        String token = IOUtils.toString(getClass().getResourceAsStream("two-node-healthy.status.json"));
        Map<String, Object> actual = admin.getStatus(token);
        assertNotNull(actual.toString());
    }
}
