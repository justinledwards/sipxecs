/*
 *
 *
 * Copyright (C) 2008 Pingtel Corp., certain elements licensed under a Contributor Agreement.
 * Contributors retain copyright to elements licensed under a Contributor Agreement.
 * Licensed to the User under the LGPL license.
 *
 * $
 */
package org.sipfoundry.sipxconfig.site.admin.time;

import java.util.Collections;
import java.util.List;

import org.apache.tapestry.annotations.Bean;
import org.apache.tapestry.annotations.InjectObject;
import org.apache.tapestry.event.PageBeginRenderListener;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.form.IPropertySelectionModel;
import org.apache.tapestry.form.StringPropertySelectionModel;
import org.sipfoundry.sipxconfig.common.CoreContext;
import org.sipfoundry.sipxconfig.common.User;
import org.sipfoundry.sipxconfig.components.SipxValidationDelegate;
import org.sipfoundry.sipxconfig.site.user_portal.UserBasePage;
import org.sipfoundry.sipxconfig.time.NtpManager;

import com.davekoelle.AlphanumComparator;

public abstract class EditTimeZonePage extends UserBasePage implements PageBeginRenderListener {
    public static final String PAGE = "admin/time/EditTimeZonePage";

    public abstract User getEditedUser();

    public abstract void setEditedUser(User user);

    @InjectObject("spring:ntpManager")
    public abstract NtpManager getTimeManager();

    @InjectObject("spring:coreContext")
    public abstract CoreContext getCoreContext();

    public abstract String getTimezoneType();

    public abstract void setTimezoneType(String type);

    public abstract IPropertySelectionModel getTimezoneTypeModel();

    public abstract void setTimezoneTypeModel(IPropertySelectionModel model);

    @Bean
    public abstract SipxValidationDelegate getValidator();

    public void pageBeginRender(PageEvent event_) {
        if (getEditedUser() == null) {
            setEditedUser(getUser());
        }
        // Init. the timezone dropdown menu.
        List<String> timezoneList = getTimeManager().getAvailableTimezones();

        // Sort list alphanumerically.
        Collections.sort(timezoneList, new AlphanumComparator());
        StringPropertySelectionModel model = new StringPropertySelectionModel(
                timezoneList.toArray(new String[timezoneList.size()]));
        setTimezoneTypeModel(model);
        if (!event_.getRequestCycle().isRewinding()) {
            setTimezoneType(getUser().getTimezone().getID());
        }
    }

    public boolean isRenderBranchOption() {
        return getUser().getUserBranch() != null;
    }

    public boolean isRenderTimeZoneDropDown() {
        return getUser().getInheritedBranch() == null
                || !(Boolean) getEditedUser().getSettings().getSetting("timezone/useBranchTimezone").getTypedValue();
    }

    public void onApply() {
        getEditedUser().setSettingValue("timezone/timezone", getTimezoneType());
        getCoreContext().saveUser(getEditedUser());
    }

    public void setSysTimezone() {
        getTimeManager().setSystemTimezone(getTimezoneType());
    }

}
