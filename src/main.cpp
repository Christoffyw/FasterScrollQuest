#include "PluginConfig.hpp"
#include "main.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/UI/CanvasScaler.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/CanvasRenderer.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Color.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Monobehaviour.hpp"
#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "custom-types/shared/types.hpp"
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"

#include <iostream>
#include <string>
using namespace std;
using namespace HMUI;
using namespace GlobalNamespace;
using namespace QuestUI;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

DEFINE_TYPE(FasterScroll, Main);

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration configuration(modInfo);
    return configuration;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load();
    getPluginConfig().Init(info);
    getConfig().Reload();
    getConfig().Write();

    getLogger().info("Completed setup!");
}

float m_fStockScrollSpeed;
std::string str = "LevelsTableView";
LevelCollectionTableView* levelCollectionTableView;

void FasterScroll::Main::Update() {
    if(levelCollectionTableView != nullptr) {
        TableView* tv = levelCollectionTableView->tableView;
        ScrollView* sv = tv->GetComponent<ScrollView*>();

        if (to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())) == str) {
            sv->joystickScrollSpeed = getPluginConfig().CustomScrollSpeed.GetValue();
            getLogger().info("ScrollViewPatcherConstant");
        }
        else {
            getLogger().info("%s", to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())).c_str());
        }
    }
}

void SetStockScrollSpeed(ScrollView* sv) {
    m_fStockScrollSpeed = sv->joystickScrollSpeed;
    getLogger().info("SetStockScrollSpeed");
}

void ScrollViewPatcherConstant(LevelCollectionTableView* lctv)
{
    TableView* tv = lctv->tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())) == str) {
        sv->joystickScrollSpeed = getPluginConfig().CustomScrollSpeed.GetValue();
        getLogger().info("ScrollViewPatcherConstant");
    }
    else {
        getLogger().info("%s", to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())).c_str());
    }
}

void ScrollViewPatcherStock(LevelCollectionTableView* lctv)
{
    getLogger().info("ScrollViewPatcherStock");
    TableView* tv = lctv->tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (sv->get_transform()->get_parent()->get_gameObject()->get_name() == il2cpp_utils::newcsstr("LevelsTableView"))
        sv->joystickScrollSpeed = m_fStockScrollSpeed;
}

MAKE_HOOK_MATCH(ScrollView_Awake, &HMUI::ScrollView::Awake, void, HMUI::ScrollView* self) {
    SetStockScrollSpeed(self);
    ScrollView_Awake(self);
}

MAKE_HOOK_MATCH(LevelCollectionTableView_OnEnable, &GlobalNamespace::LevelCollectionTableView::OnEnable, void, GlobalNamespace::LevelCollectionTableView* self) {
    levelCollectionTableView = self;
    ScrollViewPatcherConstant(self);
    LevelCollectionTableView_OnEnable(self);
}

#include "GlobalNamespace/MainMenuViewController.hpp"
MAKE_HOOK_MATCH(MainMenuViewController_DidActivate, &GlobalNamespace::MainMenuViewController::DidActivate, void, GlobalNamespace::MainMenuViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation) {
        GameObject* fasterScrollGO = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("FasterScrollGO"));
        fasterScrollGO->get_transform()->set_position(UnityEngine::Vector3(0, 0, 0));
        fasterScrollGO->get_gameObject()->AddComponent<FasterScroll::Main*>();
    }
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    getLogger().info("DidActivate: %p, %d, %d, %d", self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(firstActivation) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());



        auto* textGrid = container;
        //        textGrid->set_spacing(1);

        QuestUI::BeatSaberUI::CreateText(textGrid->get_transform(), "Faster Scroll Mod settings.");

        //        buttonsGrid->set_spacing(1);

        auto* floatGrid = container;

        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().CustomScrollSpeed, 0, 10, 30, 500)->get_gameObject(),"Changes the speed of scrolling with the joystick when selecting songs. Default: 60");
    }
}

extern "C" void load() {
  il2cpp_functions::Init();
  QuestUI::Init();
  QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
  INSTALL_HOOK(getLogger(), MainMenuViewController_DidActivate);
  INSTALL_HOOK(getLogger(), LevelCollectionTableView_OnEnable);
  INSTALL_HOOK(getLogger(), ScrollView_Awake);
  custom_types::Register::AutoRegister();
}