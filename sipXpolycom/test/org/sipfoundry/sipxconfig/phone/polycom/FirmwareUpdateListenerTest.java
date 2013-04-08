package org.sipfoundry.sipxconfig.phone.polycom;

import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.SETTING_TRANSPORT_REG_PROXY;
import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.SETTING_TRANSPORT_REG_SERV1;
import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.SETTING_TRANSPORT_REG_SERV2;
import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.SETTING_TRANSPORT_VOIPPROT;
import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.TCP_ONLY_32;
import static org.sipfoundry.sipxconfig.phone.polycom.FirmwareUpdateListener.TCP_ONLY_40;

import org.sipfoundry.sipxconfig.phone.Line;
import org.sipfoundry.sipxconfig.phone.Phone;
import org.sipfoundry.sipxconfig.phone.PhoneContext;
import org.sipfoundry.sipxconfig.phone.PhoneModel;
import org.sipfoundry.sipxconfig.test.IntegrationTestCase;

public class FirmwareUpdateListenerTest extends IntegrationTestCase {
    private PhoneContext m_phoneContext;

    //disabled; can't figure out how to run integration tests from plugins
    public void testSettingValueChanges() {
        /*PhoneModel model = new PolycomModel();
        model.setBeanId("polycom");
        model.setModelId("polycom");
        PolycomPhone phone = (PolycomPhone) m_phoneContext.newPhone(model);
        
        phone.setUniqueId(1001);
        phone.setDeviceVersion(PolycomModel.VER_3_2_X);
        phone.setSettingValue(SETTING_TRANSPORT_VOIPPROT, TCP_ONLY_32);
        Line line = phone.createLine();
        line.setSettingValue(SETTING_TRANSPORT_REG_PROXY, TCP_ONLY_32);
        line.setSettingValue(SETTING_TRANSPORT_REG_SERV1, TCP_ONLY_32);
        line.setSettingValue(SETTING_TRANSPORT_REG_SERV2, TCP_ONLY_32);
        m_phoneContext.storeLine(line);
        m_phoneContext.storePhone(phone);
        
        Phone loadedPhone = m_phoneContext.loadPhone(1001);
        assertEquals(TCP_ONLY_32, loadedPhone.getSettingValue(SETTING_TRANSPORT_VOIPPROT));
        Line loadedLine = loadedPhone.getLine(1);
        assertEquals(TCP_ONLY_32, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_PROXY));
        assertEquals(TCP_ONLY_32, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_SERV1));
        assertEquals(TCP_ONLY_32, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_SERV2));
        
        loadedPhone.setDeviceVersion(PolycomModel.VER_4_0_X);
        m_phoneContext.storePhone(loadedPhone);

        loadedPhone = m_phoneContext.loadPhone(1001);
        assertEquals(TCP_ONLY_40, loadedPhone.getSettingValue(SETTING_TRANSPORT_VOIPPROT));
        loadedLine = loadedPhone.getLine(1);
        assertEquals(TCP_ONLY_40, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_PROXY));
        assertEquals(TCP_ONLY_40, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_SERV1));
        assertEquals(TCP_ONLY_40, loadedLine.getSettingValue(SETTING_TRANSPORT_REG_SERV2));*/
    }

    public void setPhoneContext(PhoneContext phoneContext) {
        m_phoneContext = phoneContext;
    }
}
