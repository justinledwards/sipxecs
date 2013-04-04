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
package org.sipfoundry.sipxconfig.common;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;

import org.apache.commons.io.IOUtils;
import org.apache.commons.io.output.NullOutputStream;
import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Run commands and allow caller to block for a specific time and unblock for
 * a specific remaining time before killing process.
 */
public class SimpleCommandRunner {
    private static final Log LOG = LogFactory.getLog(SimpleCommandRunner.class);
    private ByteArrayOutputStream m_stderr;
    private ByteArrayOutputStream m_stdout;
    private String m_stdin;
    private Thread m_inThread;
    private Thread m_outThread;
    private Thread m_errThread;
    private Thread m_procThread;
    private volatile Integer m_exitCode;

    public String getStderr() {
        return m_stderr.toString();
    }

    public String getStdout() {
        return m_stdout.toString();
    }

    public void setStdin(String stdin) {
        m_stdin = stdin;
    }

    public String getStdin() {
        return m_stdin;
    }

    public Integer getExitCode() {
        return m_exitCode;
    }

    public boolean isInProgress() {
        return m_procThread != null && m_procThread.isAlive();
    }

    public boolean run(String[] command, final int foregroundTimeout) {
        return run(command, foregroundTimeout, 0);
    }

    public boolean run(String[] command, final int foregroundTimeout, final int backgroundTimeout) {
        // clean-up in case this is reused
        kill();
        m_stderr = new ByteArrayOutputStream();
        m_stdout = new ByteArrayOutputStream();
        m_exitCode = null;
        m_inThread = null;

        final String fullCommand = StringUtils.join(command, ' ');
        LOG.info(fullCommand);
        ProcessBuilder pb = new ProcessBuilder(Arrays.asList(command));
        try {
            final Process p = pb.start();
            if (m_stdin != null) {
                Runnable inPutter = new Runnable() {
                    @Override
                    public void run() {
                        InputStream in = new ByteArrayInputStream(m_stdin.getBytes());
                        try {
                            IOUtils.copy(in, p.getOutputStream());
                            in.close();
                            // need to close otherwise process hangs.
                            p.getOutputStream().close();
                        } catch (IOException e) {
                            LOG.error("Closing input stream of command runner. " + fullCommand, e);
                        }
                    }
                };
                m_inThread = new Thread(null, inPutter, "CommandRunner-stdin");
                m_inThread.start();
            }
            StreamGobbler outGobbler = new StreamGobbler(p.getInputStream(), m_stdout);
            m_outThread = new Thread(null, outGobbler, "CommandRunner-stdout");
            m_outThread.start();
            StreamGobbler errGobbler = new StreamGobbler(p.getErrorStream(), m_stderr);
            m_errThread = new Thread(null, errGobbler, "CommandRunner-stderr");
            m_errThread.start();
            Runnable procRunner = new Runnable() {
                @Override
                public void run() {
                    try {
                        m_exitCode = p.waitFor();
                    } catch (InterruptedException willBeHandledByReaper) {
                        LOG.error("Interrupted running " + fullCommand);
                    }
                }
            };
            m_procThread = new Thread(null, procRunner, "CommandRunner-process");
            m_procThread.start();

            m_procThread.join(foregroundTimeout);

            // all ok
            if (m_exitCode != null) {
                return true;
            }

            // not requested to go into background
            if (backgroundTimeout <= foregroundTimeout) {
                LOG.info("background timer not specified or valid, killing process " + fullCommand);
                kill();
                return false;
            }

            final int remainingTime = backgroundTimeout - foregroundTimeout;
            LOG.info("putting process in background for " + remainingTime + " ms " + fullCommand);
            // schedule process to be killed after background timeout
            Runnable reaper = new Runnable() {
                @Override
                public void run() {
                    try {
                        // already waited foreground, so subtract that off
                        m_procThread.join(remainingTime);
                        if (m_exitCode == null) {
                            LOG.info("Reaping background process, did not complete in time. " + fullCommand);
                            kill();
                        }
                    } catch (InterruptedException e) {
                        throw new UserException(e);
                    }
                }
            };
            Thread reaperThread = new Thread(null, reaper, "CommandRunner-reaper");
            reaperThread.start();
            return false;
        } catch (IOException e) {
            throw new UserException(e);
        } catch (InterruptedException e1) {
            throw new UserException(e1);
        }
    }

    void kill() {
        for (Thread t : new Thread[] {
            m_inThread, m_procThread, m_errThread, m_outThread
        }) {
            if (t != null && t.isAlive()) {
                t.interrupt();
            }
        }
        m_inThread = null;
        m_procThread = null;
        m_errThread = null;
        m_outThread = null;
    }

    class StreamGobbler implements Runnable {
        private InputStream m_in;
        private OutputStream m_out;
        private IOException m_error;

        StreamGobbler(InputStream in) {
            m_in = in;
            m_out = new NullOutputStream();
        }

        StreamGobbler(InputStream in, OutputStream out) {
            m_in = in;
            m_out = out;
        }

        @Override
        public void run() {
            try {
                IOUtils.copy(m_in, m_out);
            } catch (IOException e) {
                m_error = e;
            }
        }
    }
}
