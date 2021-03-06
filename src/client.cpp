/*
 *      Copyright (C) 2015 Sam Stenvall
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <algorithm>
#include "util/util.h"
#include "xbmc_pvr_dll.h"
#include "client.h"
#include "vbox/Exceptions.h"
#include "vbox/VBox.h"
#include "vbox/timeshift/DummyBuffer.h"
#include "vbox/timeshift/FilesystemBuffer.h"
#include "xmltv/Utilities.h"

using namespace ADDON;
using namespace vbox;

// Initialize helpers
CHelper_libXBMC_addon *XBMC = NULL;
CHelper_libXBMC_pvr   *PVR = NULL;

// Initialize globals
ADDON_STATUS   g_status = ADDON_STATUS_UNKNOWN;
VBox *g_vbox = nullptr;
timeshift::Buffer *timeshiftBuffer = nullptr;
std::string g_hostname;
std::string g_externalIp;
int g_port;
bool g_useExternalXmltv;
std::string g_externalXmltvPath;
bool g_preferExternalXmltv;
bool g_timeshiftEnabled;
std::string g_timeshiftBufferPath;

extern "C" {

  void ADDON_ReadSettings()
  {
#define UPDATE_INT(var, key, def)\
  if (!XBMC->GetSetting(key, &var))\
    var = def;

#define UPDATE_STR(var, key, tmp, def)\
  if (XBMC->GetSetting(key, tmp))\
    var = tmp;\
    else\
    var = def;

    char buffer[1024];

    UPDATE_STR(g_hostname, "hostname", buffer, "");
    UPDATE_STR(g_externalIp, "external_ip", buffer, "");
    UPDATE_INT(g_port, "port", 80);
    UPDATE_INT(g_useExternalXmltv, "use_external_xmltv", false);
    UPDATE_STR(g_externalXmltvPath, "external_xmltv_path", buffer, "");
    UPDATE_INT(g_preferExternalXmltv, "prefer_external_xmltv", true);
    UPDATE_INT(g_timeshiftEnabled, "timeshift_enabled", false);
    UPDATE_STR(g_timeshiftBufferPath, "timeshift_path", buffer, "");

#undef UPDATE_INT
#undef UPDATE_STR
  }

  ADDON_STATUS ADDON_Create(void* hdl, void* props)
  {
    if (!hdl || !props)
      return ADDON_STATUS_UNKNOWN;

    PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

    // Instantiate helpers
    XBMC = new CHelper_libXBMC_addon;
    PVR = new CHelper_libXBMC_pvr;

    if (!XBMC->RegisterMe(hdl) ||
      !PVR->RegisterMe(hdl))
    {
      SAFE_DELETE(XBMC);
      SAFE_DELETE(PVR);
      return ADDON_STATUS_PERMANENT_FAILURE;
    }

    // Read the settings. The addon is restarted whenever a setting is changed 
    // so we only need to read them here.
    ADDON_ReadSettings();
    Settings settings;
    settings.m_hostname = g_hostname;
    settings.m_externalIp = g_externalIp;
    settings.m_port = g_port;
    settings.m_useExternalXmltv = g_useExternalXmltv;
    settings.m_externalXmltvPath = g_externalXmltvPath;
    settings.m_preferExternalXmltv = g_preferExternalXmltv;
    settings.m_timeshiftEnabled = g_timeshiftEnabled;
    settings.m_timeshiftBufferPath = g_timeshiftBufferPath;

    // Create the addon
    VBox::Log(LOG_DEBUG, "creating VBox Gateway PVR addon");
    g_status = ADDON_STATUS_UNKNOWN;
    g_vbox = new VBox(settings);

    // Validate settings
    if (g_vbox->ValidateSettings())
    {
      // Start the addon
      try {
        g_vbox->Initialize();
        g_status = ADDON_STATUS_OK;
      }
      catch (FirmwareVersionException &e) {
        XBMC->QueueNotification(ADDON::QUEUE_ERROR, e.what());
        g_status = ADDON_STATUS_PERMANENT_FAILURE;
      }
      catch (VBoxException &e) {
        VBox::LogException(e);
        g_status = ADDON_STATUS_LOST_CONNECTION;
      }
    }
    else
      g_status = ADDON_STATUS_NEED_SETTINGS;

    return g_status;
  }

  ADDON_STATUS ADDON_GetStatus()
  {
    return g_status;
  }

  void ADDON_Destroy()
  {
    SAFE_DELETE(g_vbox);
    g_status = ADDON_STATUS_UNKNOWN;
  } 

  bool ADDON_HasSettings()
  {
    return true;
  }

  unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
  {
    return 0;
  }

  ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
  {
#define UPDATE_STR(key, var)\
  if (!strcmp(settingName, key))\
  {\
    if (!strcmp(var.c_str(), (const char*)settingValue))\
    {\
      VBox::Log(LOG_INFO, "updated setting %s from '%s' to '%s'",\
        settingName, var.c_str(), settingValue);\
      return ADDON_STATUS_NEED_RESTART;\
    }\
    return ADDON_STATUS_OK;\
  }

#define UPDATE_INT(key, type, var)\
  if (!strcmp(settingName, key))\
  {\
    if (var != *(type*)settingValue)\
    {\
      VBox::Log(LOG_INFO, "updated setting %s from '%d' to '%d'",\
        settingName, var, (int)*(type*)settingValue);\
      return ADDON_STATUS_NEED_RESTART;\
    }\
    return ADDON_STATUS_OK;\
  }

    UPDATE_STR("hostname", g_hostname);
    UPDATE_STR("external_ip", g_externalIp);
    UPDATE_INT("port", int, g_port);
    UPDATE_INT("use_external_xmltv", int, g_useExternalXmltv);
    UPDATE_STR("external_xmltv_path", g_externalXmltvPath);
    UPDATE_INT("prefer_external_xmltv", int, g_preferExternalXmltv);
    UPDATE_INT("timeshift_enabled", int, g_timeshiftEnabled);
    UPDATE_STR("timeshift_path", g_timeshiftBufferPath);

    return ADDON_STATUS_OK;
#undef UPDATE_INT
#undef UPDATE_STR
  }

  void ADDON_Stop()
  {
  }

  void ADDON_FreeSettings()
  {
  }

  void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
  {
  }

  const char* GetPVRAPIVersion(void)
  {
    static const char *strApiVersion = XBMC_PVR_API_VERSION;
    return strApiVersion;
  }

  const char* GetMininumPVRAPIVersion(void)
  {
    static const char *strMinApiVersion = XBMC_PVR_MIN_API_VERSION;
    return strMinApiVersion;
  }

  const char* GetGUIAPIVersion(void)
  {
    static const char *strGuiApiVersion = KODI_GUILIB_API_VERSION;
    return strGuiApiVersion;
  }

  const char* GetMininumGUIAPIVersion(void)
  {
    static const char *strMinGuiApiVersion = KODI_GUILIB_MIN_API_VERSION;
    return strMinGuiApiVersion;
  }

  PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
  {
    pCapabilities->bSupportsTV = true;
    pCapabilities->bSupportsRadio = true;
    pCapabilities->bSupportsChannelGroups = false;
    pCapabilities->bSupportsEPG = true;
    pCapabilities->bHandlesInputStream = true;

    // Recording capability is determined further down, we'll assume false 
    // in case the real capabilities cannot be determined for some reason
    pCapabilities->bSupportsRecordings = false;
    pCapabilities->bSupportsTimers = false;

    // Unsupported features
    pCapabilities->bSupportsRecordingsUndelete = false;
    pCapabilities->bSupportsChannelScan = false;
    pCapabilities->bSupportsChannelSettings = false;
    pCapabilities->bHandlesDemuxing = false;
    pCapabilities->bSupportsRecordingFolders = false;
    pCapabilities->bSupportsRecordingPlayCount = false;
    pCapabilities->bSupportsLastPlayedPosition = false;
    pCapabilities->bSupportsRecordingEdl = false;

    // Wait for initialization until we decide if we support recordings or not.
    // Recording is only possible when external media is present
    if (g_vbox->GetStateHandler().WaitForState(StartupState::INITIALIZED)
      && g_vbox->SupportsRecordings())
    {
      pCapabilities->bSupportsRecordings = true;
      pCapabilities->bSupportsTimers = true;
    }

    return PVR_ERROR_NO_ERROR;
  }

  const char *GetBackendName(void)
  {
    static std::string backendName = g_vbox->GetBackendName();
    return backendName.c_str();
  }

  const char *GetBackendVersion(void)
  {
    static std::string backendVersion = g_vbox->GetBackendVersion();
    return backendVersion.c_str();
  }

  const char *GetConnectionString(void)
  {
    static std::string connectionString = g_vbox->GetConnectionString();
    return connectionString.c_str();
  }

  const char *GetBackendHostname(void)
  {
    static std::string backendHostname = g_vbox->GetBackendHostname();
    return backendHostname.c_str();
  }

  int GetChannelsAmount(void)
  {
    try {
      return g_vbox->GetChannelsAmount();
    }
    catch (VBoxException &e)
    {
      g_vbox->LogException(e);
      return PVR_ERROR_SERVER_ERROR;
    }
  }

  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio)
  {
    auto &channels = g_vbox->GetChannels();

    for (const auto &item : channels)
    {
      // Skip those that are not of the correct type
      if (item->m_radio != bRadio)
        continue;

      PVR_CHANNEL channel;
      memset(&channel, 0, sizeof(PVR_CHANNEL));

      channel.iUniqueId = item->GetUniqueId();
      channel.bIsRadio = item->m_radio;
      channel.iChannelNumber = item->m_number;
      channel.iEncryptionSystem = item->m_encrypted ? 0xFFFF : 0x0000;

      strncpy(channel.strChannelName, item->m_name.c_str(),
        sizeof(channel.strChannelName));
      strncpy(channel.strIconPath, item->m_iconUrl.c_str(),
        sizeof(channel.strIconPath));

      // Set stream format for TV channels
      if (!item->m_radio)
      {
        strncpy(channel.strInputFormat, "video/mp2t",
          sizeof(channel.strInputFormat));
      }

      PVR->TransferChannelEntry(handle, &channel);
    }

    return PVR_ERROR_NO_ERROR;
  }

  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
  {
    *iTotal = g_vbox->GetRecordingTotalSpace();
    *iUsed = g_vbox->GetRecordingUsedSpace();

    return PVR_ERROR_NO_ERROR;
  }

  int GetRecordingsAmount(bool deleted)
  {
    return g_vbox->GetRecordingsAmount();
  }

  PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted)
  {
    auto &recordings = g_vbox->GetRecordingsAndTimers();

    for (const auto &item : recordings)
    {
      // Skip timers
      if (!item->IsRecording())
        continue;

      PVR_RECORDING recording;
      memset(&recording, 0, sizeof(PVR_RECORDING));

      recording.recordingTime = item->m_start;
      recording.iDuration = static_cast<int>(item->m_end - item->m_start);

      strncpy(recording.strChannelName, item->m_channelName.c_str(),
        sizeof(recording.strChannelName));

      strncpy(recording.strRecordingId, std::to_string(item->m_id).c_str(),
        sizeof(recording.strRecordingId));

      strncpy(recording.strStreamURL, item->m_url.c_str(),
        sizeof(recording.strStreamURL));

      strncpy(recording.strTitle, item->m_title.c_str(),
        sizeof(recording.strTitle));

      strncpy(recording.strPlot, item->m_description.c_str(),
        sizeof(recording.strPlot));

      PVR->TransferRecordingEntry(handle, &recording);
    }

    return PVR_ERROR_NO_ERROR;
  }

  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
  {
    // DeleteRecording swallows its exceptions, but std::stoul throws if the 
    // value can't be converted
    try {
      unsigned int id = std::stoul(recording.strRecordingId);

      if (g_vbox->DeleteRecordingOrTimer(id))
      {
        PVR->TriggerRecordingUpdate();
        return PVR_ERROR_NO_ERROR;
      }
      else
        return PVR_ERROR_FAILED;
    }
    catch (...)
    {
      return PVR_ERROR_INVALID_PARAMETERS;
    }
  }

  int GetTimersAmount(void)
  {
    return g_vbox->GetTimersAmount();
  }

  PVR_ERROR GetTimers(ADDON_HANDLE handle)
  {
    auto &recordings = g_vbox->GetRecordingsAndTimers();

    for (const auto &item : recordings)
    {
      // Skip recordings
      if (!item->IsTimer())
        continue;

      PVR_TIMER timer;
      memset(&timer, 0, sizeof(PVR_TIMER));

      timer.startTime = item->m_start;
      timer.endTime = item->m_end;
      timer.iClientIndex = item->m_id;

      // Convert the internal timer state to PVR_TIMER_STATE
      switch (item->GetState())
      {
      case RecordingState::SCHEDULED:
        timer.state = PVR_TIMER_STATE_SCHEDULED;
        break;
      case RecordingState::RECORDED:
      case RecordingState::EXTERNAL:
        timer.state = PVR_TIMER_STATE_COMPLETED;
        break;
      case RecordingState::RECORDING:
        timer.state = PVR_TIMER_STATE_RECORDING;
        break;
      }

      // Find the timer's channel and use its unique ID
      auto &channels = g_vbox->GetChannels();
      auto it = std::find_if(channels.cbegin(), channels.cend(),
        [&item](const ChannelPtr &channel)
      {
        return channel->m_xmltvName == item->m_channelId;
      });

      if (it != channels.cend())
        timer.iClientChannelUid = (*it)->GetUniqueId();

      strncpy(timer.strTitle, item->m_title.c_str(),
        sizeof(timer.strTitle));

      strncpy(timer.strSummary, item->m_description.c_str(),
        sizeof(timer.strSummary));

      // TODO: Set margins to whatever the API reports
      PVR->TransferTimerEntry(handle, &timer);
    }

    return PVR_ERROR_NO_ERROR;
  }

  PVR_ERROR AddTimer(const PVR_TIMER &timer)
  {
    // Find the channel the timer is for
    auto &channels = g_vbox->GetChannels();
    auto it = std::find_if(channels.cbegin(), channels.cend(),
      [&timer](const ChannelPtr &channel)
    {
      return channel->GetUniqueId() == timer.iClientChannelUid;
    });

    if (it == channels.end())
      return PVR_ERROR_INVALID_PARAMETERS;

    const Channel *channel = it->get();

    // Find the event the timer is for
    const xmltv::Programme *programme = g_vbox->GetProgramme(timer.iEpgUid);

    if (!programme)
      return PVR_ERROR_INVALID_PARAMETERS;

    // Add the timer
    try {
      g_vbox->AddTimer(channel, programme);
    }
    catch (VBoxException &e)
    {
      g_vbox->LogException(e);
      return PVR_ERROR_FAILED;
    }

    // Make sure Kodi sees the new items
    PVR->TriggerTimerUpdate();
    PVR->TriggerRecordingUpdate();

    return PVR_ERROR_NO_ERROR;
  }

  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
  {
    if (g_vbox->DeleteRecordingOrTimer(timer.iClientIndex))
    {
      PVR->TriggerTimerUpdate();
      return PVR_ERROR_NO_ERROR;
    }

    return PVR_ERROR_FAILED;
  }

  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
  {
    const Channel *channelPtr = g_vbox->GetChannel(channel.iUniqueId);

    if (!channelPtr)
      return PVR_ERROR_INVALID_PARAMETERS;

    // Retrieve the schedule and filter out the programmes that don't fit 
    // within the start and end times
    const auto *schedule = g_vbox->GetSchedule(channelPtr);

    if (!schedule)
      return PVR_ERROR_NO_ERROR;
      
    std::string xmltvStartTime = xmltv::Utilities::UnixTimeToXmltv(iStart);
    std::string xmltvEndTime = xmltv::Utilities::UnixTimeToXmltv(iEnd);

    auto it = std::find_if(
      schedule->cbegin(),
      schedule->cend(),
      [xmltvStartTime, xmltvEndTime](const xmltv::ProgrammePtr &programme)
    {
      return programme->m_startTime >= xmltvStartTime &&
        programme->m_endTime <= xmltvEndTime;
    });

    // Transfer the events
    while (it != schedule->cend())
    {
      const auto &programme = *it;
      EPG_TAG event;
      memset(&event, 0, sizeof(EPG_TAG));

      event.startTime = xmltv::Utilities::XmltvToUnixTime(programme->m_startTime);
      event.endTime = xmltv::Utilities::XmltvToUnixTime(programme->m_endTime);
      event.iChannelNumber = channel.iChannelNumber;
      event.iUniqueBroadcastId = programme->GetUniqueId();
      event.strTitle = programme->m_title.c_str();
      event.strPlot = programme->m_description.c_str();
      event.strDirector = xmltv::Utilities::ConcatenateStringList(programme->GetDirectors()).c_str();
      event.strWriter = xmltv::Utilities::ConcatenateStringList(programme->GetWriters()).c_str();
      event.iYear = programme->m_year;
      event.strGenreDescription = xmltv::Utilities::ConcatenateStringList(programme->GetCategories()).c_str();
      event.strEpisodeName = programme->m_subTitle.c_str();

      // Extract up to five cast members only
      std::vector<std::string> actorNames;
      const auto &actors = programme->GetActors();
      int numActors = std::min(static_cast<int>(actors.size()), 5);

      for (int i = 0; i < numActors; i++)
        actorNames.push_back(actors.at(i).name);

      event.strCast = xmltv::Utilities::ConcatenateStringList(actorNames).c_str();

      PVR->TransferEpgEntry(handle, &event);
      it++;
    }

    return PVR_ERROR_NO_ERROR;
  }

  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus)
  {
    const Channel *currentChannel = g_vbox->GetCurrentChannel();

    if (currentChannel)
    {
      try {
        ChannelStreamingStatus status = g_vbox->GetChannelStreamingStatus(currentChannel);

        // Adjust for Kodi's weird handling of the signal strength
        signalStatus.iSignal = static_cast<int>(status.GetSignalStrength()) * 655; 
        signalStatus.iSNR = static_cast<int>(status.m_signalQuality) * 655;
        signalStatus.iBER = status.GetBer();

        g_vbox->Log(LOG_INFO, "STATUS: signal %d, snr %d", signalStatus.iSignal, signalStatus.iSNR);

        strncpy(signalStatus.strAdapterName,
          status.GetTunerName().c_str(), sizeof(signalStatus.strAdapterName));

        strncpy(signalStatus.strAdapterStatus,
          status.m_lockStatus.c_str(), sizeof(signalStatus.strAdapterStatus));
        
        strncpy(signalStatus.strServiceName,
          status.GetServiceName().c_str(), sizeof(signalStatus.strServiceName));

        strncpy(signalStatus.strMuxName,
          status.m_frequency.c_str(), sizeof(signalStatus.strMuxName));
      }
      catch (VBoxException &e)
      {
        g_vbox->LogException(e);
      }
    }

    return PVR_ERROR_NO_ERROR;
  }

  bool OpenLiveStream(const PVR_CHANNEL &channel)
  {
    // Find the channel
    const Channel* channelPtr = g_vbox->GetChannel(channel.iUniqueId);
    
    if (!channelPtr)
      return false;

    // Reset the buffer
    SAFE_DELETE(timeshiftBuffer);

    // Create and open a new timeshift buffer
    if (g_vbox->GetSettings().m_timeshiftEnabled)
    {
      const std::string &bufferPath = g_vbox->GetSettings().m_timeshiftBufferPath;
      timeshiftBuffer = new timeshift::FilesystemBuffer(bufferPath);
    }
    else
      timeshiftBuffer = new timeshift::DummyBuffer();
    
    // Remember the current channel if the buffer was successfully opened
    if (timeshiftBuffer->Open(channelPtr->m_url))
    {
      g_vbox->SetCurrentChannel(channelPtr);
      return true;
    }

    // Close the buffer if it failed to open
    SAFE_DELETE(timeshiftBuffer);
    return false;
  }

  void CloseLiveStream(void)
  {
    SAFE_DELETE(timeshiftBuffer);

    // Unset the current channel
    g_vbox->SetCurrentChannel(nullptr);
  }

  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
  {
    return timeshiftBuffer->Read(pBuffer, iBufferSize);
  }

  long long PositionLiveStream(void)
  {
    return timeshiftBuffer->Position();
  }

  long long LengthLiveStream(void)
  {
    return timeshiftBuffer->Length();
  }

  long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */)
  {
    return timeshiftBuffer->Seek(iPosition, iWhence);
  }

  int GetCurrentClientChannel(void)
  {
    // TODO: Investigate whether Kodi actually uses this method anymore
    const Channel *channel = g_vbox->GetCurrentChannel();

    if (channel)
      return channel->GetUniqueId();
    
    return -1;
  }

  bool CanPauseStream(void)
  {
    return g_vbox->GetSettings().m_timeshiftEnabled;
  }

  bool CanSeekStream(void)
  {
    return g_vbox->GetSettings().m_timeshiftEnabled;
  }

  time_t GetBufferTimeStart()
  {
    return timeshiftBuffer->GetStartTime();
  }

  time_t GetBufferTimeEnd()
  {
    return timeshiftBuffer->GetEndTime();
  }

  // Management methods
  PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook, const PVR_MENUHOOK_DATA &item) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR OpenDialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording) { return PVR_ERROR_NOT_IMPLEMENTED; }

  // Channel group methods
  int GetChannelGroupsAmount(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group) { return PVR_ERROR_NOT_IMPLEMENTED; }

  // Recording stream methods
  bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
  void CloseRecordedStream(void) {}
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
  long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
  long long PositionRecordedStream(void) { return -1; }
  long long LengthRecordedStream(void) { return 0; }

  // Channel stream methods
  bool SwitchChannel(const PVR_CHANNEL &channel) { CloseLiveStream(); return OpenLiveStream(channel); }

  // Demuxer methods
  void DemuxReset(void) {}
  void DemuxFlush(void) {}
  void DemuxAbort(void) {}
  DemuxPacket* DemuxRead(void) { return NULL; }
  PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; }
  
  // Recording methods (not supported by VBox)
  PVR_ERROR RenameRecording(const PVR_RECORDING &recording) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING &recording, int count) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING &recording, int lastplayedposition) { return PVR_ERROR_NOT_IMPLEMENTED; }
  int GetRecordingLastPlayedPosition(const PVR_RECORDING &recording) { return -1; }
  PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY[], int*) { return PVR_ERROR_NOT_IMPLEMENTED; };
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer) { return PVR_ERROR_NOT_IMPLEMENTED; }
  PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }

  // Miscellaneous unimplemented methods
  unsigned int GetChannelSwitchDelay(void) { return 0; }

  // Timeshift methods
  void PauseStream(bool bPaused) {}
  bool SeekTime(int, bool, double*) { return false; }
  void SetSpeed(int) {};
  time_t GetPlayingTime() { return 0; }

  // Deprecated (unused)
  const char * GetLiveStreamURL(const PVR_CHANNEL &channel) { return ""; }
}
