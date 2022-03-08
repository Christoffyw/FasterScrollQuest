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
#include "UnityEngine/MonoBehaviour.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/RenderMode.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Mathf.hpp"

#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ViewController.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"
#include "HMUI/Touchable.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/TableView.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/HapticFeedbackController.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/types.hpp"
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace HMUI;
using namespace GlobalNamespace;
using namespace QuestUI;
using namespace UnityEngine;

DEFINE_TYPE(FasterScroll, Main);

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

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
float m_fInertia;
float m_fCustomSpeed; // stock value : 60.0f
float m_fScrollTimer;

bool isLinear = true;
bool isntStock = true;

HapticFeedbackController* m_oHaptic;
float m_fVanillaStockRumbleStrength; // stock value : 1.0f (will be set ONCE at launch)
float m_fRumbleStrength;
std::string str = "LevelsTableView";

void SetStockScrollSpeed(ScrollView* sv) {
    m_fStockScrollSpeed = sv->joystickScrollSpeed;
    getLogger().info("SetStockScrollSpeed");
}

void ScrollViewPatcherDynamic(ScrollView* sv)
{
    m_fScrollTimer += Time::get_deltaTime();

    if(isLinear) {
        m_fInertia = getPluginConfig().Accel.GetValue() * m_fScrollTimer;
    } else {
        return;
    }

    m_fCustomSpeed = Mathf::Clamp(m_fInertia * m_fStockScrollSpeed, 0.0f, getPluginConfig().MaxSpeed.GetValue());
    sv->joystickScrollSpeed = m_fCustomSpeed;
}

void ScrollViewPatcherConstant(LevelCollectionTableView* lctv)
{
    TableView* tv = lctv->tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())) == str) {
        sv->joystickScrollSpeed = getPluginConfig().MaxSpeed.GetValue();
        getLogger().info("ScrollViewPatcherConstant");
    }
    else {
        getLogger().info("%s", to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())).c_str());
    }
}

void ScrollViewPatcherStock(LevelCollectionTableView* lctv)
{
    TableView* tv = lctv->tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (to_utf8(csstrtostr(sv->get_transform()->get_parent()->get_gameObject()->get_name())) == str) {
        sv->joystickScrollSpeed = m_fStockScrollSpeed;
    }
}

void ResetInertia() {
    m_fInertia = 0.0f;
    m_fScrollTimer = 0.0f;
}

MAKE_HOOK_MATCH(ScrollView_Awake, &HMUI::ScrollView::Awake, void, HMUI::ScrollView* self) {
    SetStockScrollSpeed(self);
    ScrollView_Awake(self);
}

MAKE_HOOK_MATCH(LevelCollectionTableView_OnEnable, &GlobalNamespace::LevelCollectionTableView::OnEnable, void, GlobalNamespace::LevelCollectionTableView* self) {
    if(!isntStock) {
        ScrollViewPatcherStock(self);
        return;
    }
    if(!isLinear && isntStock)
        ScrollViewPatcherConstant(self);
    LevelCollectionTableView_OnEnable(self);
}

MAKE_HOOK_MATCH(ScrollView_HandleJoystickWasNotCenteredThisFrame, &HMUI::ScrollView::HandleJoystickWasNotCenteredThisFrame, void, HMUI::ScrollView* self, Vector2 deltaPos) {
    if (getPluginConfig().IsLinear.GetValue()) {
        getLogger().info("it is linear");
        if(to_utf8(csstrtostr(self->get_transform()->get_parent()->get_gameObject()->get_name())) == str && isntStock) {
            ScrollViewPatcherDynamic(self);
        }
    }
    else {
        getLogger().info("it isn't linear :(");
    }

    ScrollView_HandleJoystickWasNotCenteredThisFrame(self, deltaPos);
}

MAKE_HOOK_MATCH(ScrollView_HandleJoystickWasCenteredThisFrame, &HMUI::ScrollView::HandleJoystickWasCenteredThisFrame, void, HMUI::ScrollView* self) {
    if (isLinear && to_utf8(csstrtostr(self->get_transform()->get_parent()->get_gameObject()->get_name())) == str)
        ResetInertia();
    ScrollView_HandleJoystickWasCenteredThisFrame(self);
}

void FasterScroll::Main::Update() {
    isLinear = getPluginConfig().IsLinear.GetValue();
    isntStock = getPluginConfig().IsStock.GetValue();
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    getLogger().info("DidActivate: %p, %d, %d, %d", self, firstActivation, addedToHierarchy, screenSystemEnabling);



    if(firstActivation) {
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        self->get_gameObject()->AddComponent<FasterScroll::Main*>();
        GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        ResetInertia();
        isLinear = getPluginConfig().IsLinear.GetValue();
        isntStock = getPluginConfig().IsStock.GetValue();

        auto* textGrid = container;
        //        textGrid->set_spacing(1);

        QuestUI::BeatSaberUI::CreateText(textGrid->get_transform(), "Faster Scroll Mod settings.");
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(textGrid->get_transform(), getPluginConfig().IsStock)->get_gameObject(),"Toggles whether the mod is active or not.");

        //        buttonsGrid->set_spacing(1);

        auto* floatGrid = container;

        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().MaxSpeed, 0, 10, 30, 1000)->get_gameObject(),"Changes the speed of scrolling with the joystick. Default: 600");
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueIncrementFloat(floatGrid->get_transform(), getPluginConfig().Accel, 1, 0.1, 0.5, 5)->get_gameObject(),"Changes the acceleration speed of scrolling with the joystick. Default: 1.5");
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(floatGrid->get_transform(), getPluginConfig().IsLinear)->get_gameObject(),"Toggles whether you want to use acceleration or not.");
    }
}

extern "C" void load() {
  il2cpp_functions::Init();
  QuestUI::Init();
  QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
  //uestUI::Register::RegisterMainMenuModSettingsViewController(modInfo, DidActivate);
  INSTALL_HOOK(getLogger(), LevelCollectionTableView_OnEnable);
  INSTALL_HOOK(getLogger(), ScrollView_Awake);
  INSTALL_HOOK(getLogger(), ScrollView_HandleJoystickWasNotCenteredThisFrame);
  INSTALL_HOOK(getLogger(), ScrollView_HandleJoystickWasCenteredThisFrame);
  custom_types::Register::AutoRegister();
}