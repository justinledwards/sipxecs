package org.sipfoundry.sipxconfig.mongo;

import static org.easymock.EasyMock.createMock;
import static org.easymock.EasyMock.expectLastCall;
import static org.easymock.EasyMock.replay;
import static org.easymock.EasyMock.verify;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.feature.FeatureManager;

public class MongoOperationTest {
    private AbstractMongoOperation m_operation;
    
    @Before
    public void setUp() {
        m_operation = new AbstractMongoOperation() {
            @Override
            public void runLater(MongoOperationRequest request) {
            }
            @Override
            public boolean runNow(MongoOperationRequest request) {
                return false;
            }
            @Override
            public MongoOperation clone() {
                return super.clone();
            }
        };        
    }

    @Test
    public void testJson() throws IOException {
        InputStream in = this.getClass().getResourceAsStream("mongo-replication-status-test.json");
        List<Object> json = m_operation.readJson(in);
        assertNotNull(json);
        assertEquals(2 ,json.size());
    }
    
    @Test
    public void testHosts() throws IOException, InterruptedException {
        FeatureManager fm = createMock(FeatureManager.class);
        fm.getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
        expectLastCall().andReturn(Arrays.asList(new Location("one", "1.1.1.1")));
        fm.getLocationsForEnabledFeature(MongoManager.ARBITER_FEATURE);
        expectLastCall().andReturn(Arrays.asList(new Location("two", "2.2.2.2")));
        replay(fm);
        List<String> hosts = m_operation.getHosts(fm);        
        assertNotNull(hosts);
        assertEquals(2 ,hosts.size());
        System.out.print(hosts);
        verify(fm);
    }    
}
