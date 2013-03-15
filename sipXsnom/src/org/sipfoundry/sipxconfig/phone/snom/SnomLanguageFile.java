/*
 *
 *
 * Copyright (C) 2013 Pingtel Corp., certain elements licensed under a Contributor Agreement.
 * Contributors retain copyright to elements licensed under a Contributor Agreement.
 * Licensed to the User under the LGPL license.
 *
 */

package org.sipfoundry.sipxconfig.phone.snom;

public class SnomLanguageFile {
    private String m_type;
    private String m_fileName;
    private String m_langCode;
    private String m_version;

    public SnomLanguageFile(String type, String file, String lang, String version) {
        m_type = type;
        m_fileName = file;
        m_langCode = lang;
        m_version = version;
    }

    public void setType(String type) {
        m_type = type;
    }
    public String getType() {
        return m_type;
    }

    public void setFileName(String file) {
        m_fileName = file;
    }
    public String getFileName() {
        return m_fileName;
    }

    public void setLangCode(String lang) {
        m_langCode = lang;
    }
    public String getLangCode() {
        return m_langCode;
    }

    public void setVersion(String version) {
        m_version = version;
    }
    public String getVersion() {
        return m_version;
    }
}
