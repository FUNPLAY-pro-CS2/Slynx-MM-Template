#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <igameevents.h>
#include <ISmmPlugin.h>
#include "entitysystem.h"

namespace TemplatePlugin
{
    class TemplatePlugin : public ISmmPlugin, public IMetamodListener
    {
        void Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick);
        void Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*);
        int Hook_LoadEventsFromFile(const char* filename, bool bSearchAll);
    public:
        bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;
        bool Unload(char *error, size_t maxlen) override;
        void OnLevelShutdown() override;
        const char *GetAuthor() override;
        const char *GetName() override;
        const char *GetDescription() override;
        const char *GetURL() override;
        const char *GetLicense() override;
        const char *GetVersion() override;
        const char *GetDate() override;
        const char *GetLogTag() override;
    };

    extern TemplatePlugin g_Template;

    enum class HookMode
    {
        Pre,
        Post
    };

    enum class HookResult
    {
        Continue = 0,
        Changed = 1,
        Handled = 3,
        Stop = 4,
    };

    struct EventOverride
    {
        bool dontBroadcast = false;
    };

    using GameEventHandler = std::function<HookResult(IGameEvent* event, HookMode mode, EventOverride& override)>;

    void InitHooks();
    void ShutdownHooks();
    void RegisterGameEvent(const std::string& name, GameEventHandler handler, HookMode mode = HookMode::Post);

    HookResult OnPlayerConnectFull(IGameEvent *ev, HookMode mode, EventOverride &override);

    class CEntityListener : public IEntityListener
    {
        void OnEntityCreated(CEntityInstance* pEntity) override;
        void OnEntityDeleted(CEntityInstance* pEntity) override;
        void OnEntitySpawned(CEntityInstance* pEntity) override;
    };
}

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

PLUGIN_GLOBALVARS();
