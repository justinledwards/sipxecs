/**
 *
 *
 * Copyright (c) 2013 Sip2ser Srl.. All rights reserved.
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
