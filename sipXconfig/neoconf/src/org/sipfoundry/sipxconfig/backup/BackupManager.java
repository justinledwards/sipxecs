/**
 *
 *
 * Copyright (c) 2012 eZuce, Inc. All rights reserved.
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
package org.sipfoundry.sipxconfig.backup;

import java.io.File;
import java.util.Collection;

import org.sipfoundry.sipxconfig.commserver.Location;
import org.sipfoundry.sipxconfig.feature.FeatureManager;
import org.sipfoundry.sipxconfig.feature.GlobalFeature;

public interface BackupManager {
    public static final String CONTEXT_BEAN_NAME = "backupManager";
    public static final GlobalFeature FEATURE = new GlobalFeature("backup");

    BackupSettings getSettings();

    void saveSettings(BackupSettings settings);

    BackupPlan findOrCreateBackupPlan(BackupType type);

    Collection<BackupPlan> getBackupPlans();

    void saveBackupPlan(BackupPlan plan);

    FeatureManager getFeatureManager();

    Collection<String> getArchiveDefinitionIds();

    Collection<ArchiveDefinition> getArchiveDefinitions(Collection<String> definitionIds, Location location);

    Collection<ArchiveDefinition> getArchiveDefinitions(BackupPlan plan, Location location);

    String getBackupScript();

    File getPlanFile(BackupPlan plan);
}
