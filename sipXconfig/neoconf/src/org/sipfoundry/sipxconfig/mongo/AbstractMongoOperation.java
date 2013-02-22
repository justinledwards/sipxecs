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

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.sipfoundry.sipxconfig.common.UserException;
import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.feature.FeatureManager;

import com.mongodb.util.JSON;

public abstract class AbstractMongoOperation implements MongoOperation {
    private static final Log LOG = LogFactory.getLog(AbstractMongoOperation.class);

    public static interface CommandReader {
        public void read(AbstractMongoOperation operation, InputStream in) throws Exception;
    }

    List<String> getHosts(FeatureManager mgr) {
        List<String> hosts = new ArrayList<String>();
        List<Location> servers = mgr.getLocationsForEnabledFeature(MongoManager.FEATURE_ID);
        for (Location l : servers) {
            hosts.add(l.getFqdn() + ':' + MongoSettings.SERVER_PORT);
        }
        List<Location> arbiters = mgr.getLocationsForEnabledFeature(MongoManager.ARBITER_FEATURE);
        for (Location l : arbiters) {
            hosts.add(l.getFqdn() + ':' + MongoSettings.ARBITER_PORT);
        }
        return hosts;
    }

    @Override
    public MongoOperation clone() {
        try {
            return (MongoOperation) super.clone();
        } catch (CloneNotSupportedException e) {
            throw new RuntimeException(e);
        }
    }

    protected List<String> getReplicationCommand(MongoReplicaSetManager2 mgr, String... args) {
        List<String> cmd = new ArrayList<String>();
        cmd.add(mgr.getReplicationAdminScript());
        cmd.add("--host");
        cmd.addAll(getHosts(mgr.getFeatureManager()));
        for (String arg : args) {
            cmd.add(arg);
        }
        return cmd;
    }

    protected void run(List<String> cmd, final CommandReader reader) {
        final String cmdLine = StringUtils.join(cmd, ' ');
        LOG.debug(cmdLine);
        ProcessBuilder b = new ProcessBuilder(cmd.toArray(new String[0]));
        final Exception[] errPtr = new Exception[1];
        try {
            final Process c = b.start();
            final StringBuilder errStream = new StringBuilder();
            Thread in = null;
            if (reader != null) {
                in = new Thread(cmd.get(0) + " out") {
                    @Override
                    public void run() {
                        try {
                            reader.read(AbstractMongoOperation.this, c.getInputStream());
                        } catch (Exception e) {
                            errPtr[0] = e;
                        }
                    }
                };
                in.start();
            }
            Thread err = new Thread(cmd.get(0) + " err") {
                @Override
                public void run() {
                    try {
                        String msg = IOUtils.toString(c.getErrorStream());
                        errStream.append(msg);
                        LOG.error(cmdLine);
                        LOG.error(msg);
                    } catch (Exception e) {
                        errPtr[0] = e;
                    }
                }
            };
            err.start();
            c.waitFor();
            if (in != null) {
                in.join();
            }
            err.join();
            int rc = c.exitValue();
            if (errPtr[0] != null) {
                throw new UserException(errPtr[0]);
            }
            if (rc != 0) {
                throw new UserException("Failure to complete command, check log file. Exit code " + rc);
            }
        } catch (IOException e1) {
            throw new UserException("IO error running mongo admin operation", e1);
        } catch (InterruptedException e) {
            throw new UserException("Interrupted unning mongo admin operation", e);
        }
    }

    protected List<Object> readJson(InputStream in) throws IOException {
        String s = IOUtils.toString(in);
        @SuppressWarnings("unchecked")
        List<Object> json = (List<Object>) JSON.parse(s);
        return json;
    }
}
