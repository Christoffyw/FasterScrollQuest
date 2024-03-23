#include "Config.hpp"
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

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/types.hpp"
#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace HMUI;
using namespace GlobalNamespace;
using namespace BSML;
using namespace UnityEngine;

// Called at the early stages of game loading
MOD_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);

    getConfig().Init(modInfo);

    Logger.info("Completed setup!");
}

float m_fStockScrollSpeed;
float m_fInertia;
float m_fCustomSpeed; // stock value : 60.0f
float m_fScrollTimer;

#define isLinear (getConfig().IsLinear.GetValue())
#define isntStock (getConfig().IsStock.GetValue())
#define accel (getConfig().Accel.GetValue())
#define maxSpeed (getConfig().MaxSpeed.GetValue())

float m_fVanillaStockRumbleStrength; // stock value : 1.0f (will be set ONCE at launch)
float m_fRumbleStrength;
std::string str = "LevelsTableView";

void SetStockScrollSpeed(ScrollView* sv) {
    m_fStockScrollSpeed = sv->____joystickScrollSpeed;
}

void ScrollViewPatcherDynamic(ScrollView* sv)
{
    m_fScrollTimer += Time::get_deltaTime();

    if(!isLinear) {
        m_fInertia = accel * m_fScrollTimer;
    } else {
        return;
    }

    m_fCustomSpeed = Mathf::Clamp(m_fInertia * m_fStockScrollSpeed, 0.0f, maxSpeed);
    sv->____joystickScrollSpeed = m_fCustomSpeed;
}

void ScrollViewPatcherConstant(LevelCollectionTableView* lctv)
{
    TableView* tv = lctv->____tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (StringW(sv->get_transform()->get_parent()->get_gameObject()->get_name()) == str) {
        sv->____joystickScrollSpeed = maxSpeed;
    }
}

void ScrollViewPatcherStock(LevelCollectionTableView* lctv)
{
    TableView* tv = lctv->____tableView;
    ScrollView* sv = tv->GetComponent<ScrollView*>();

    if (StringW(sv->get_transform()->get_parent()->get_gameObject()->get_name()) == str) {
        sv->____joystickScrollSpeed = m_fStockScrollSpeed;
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
    if(isLinear && isntStock)
        ScrollViewPatcherConstant(self);
    LevelCollectionTableView_OnEnable(self);
}

MAKE_HOOK_MATCH(ScrollView_HandleJoystickWasNotCenteredThisFrame, &HMUI::ScrollView::HandleJoystickWasNotCenteredThisFrame, void, HMUI::ScrollView* self, Vector2 deltaPos) {
    if (!isLinear) {
        if(StringW(self->get_transform()->get_parent()->get_gameObject()->get_name()) == str && isntStock) {
            ScrollViewPatcherDynamic(self);
        }
    }

    ScrollView_HandleJoystickWasNotCenteredThisFrame(self, deltaPos);
}

MAKE_HOOK_MATCH(ScrollView_HandleJoystickWasCenteredThisFrame, &HMUI::ScrollView::HandleJoystickWasCenteredThisFrame, void, HMUI::ScrollView* self) {
    if (!isLinear && StringW(self->get_transform()->get_parent()->get_gameObject()->get_name()) == str)
        ResetInertia();
    ScrollView_HandleJoystickWasCenteredThisFrame(self);
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    Logger.info("DidActivate: {}, {}, {}", firstActivation, addedToHierarchy, screenSystemEnabling);

    if (!firstActivation)
        return;

    self->get_gameObject()->AddComponent<HMUI::Touchable*>();
    auto vertical = BSML::Lite::CreateVerticalLayoutGroup(self);
    vertical->set_childControlHeight(false);
    vertical->set_childForceExpandHeight(false);
    vertical->set_spacing(1);

    BSML::Lite::AddHoverHint(AddConfigValueToggle(vertical, getConfig().IsStock)->get_gameObject(),"Toggles whether the mod is active or not.");
    BSML::Lite::AddHoverHint(AddConfigValueIncrementFloat(vertical, getConfig().MaxSpeed, 0, 10, 30, 1000)->get_gameObject(),"Changes the speed of scrolling with the joystick. Default: 600");
    BSML::Lite::AddHoverHint(AddConfigValueIncrementFloat(vertical, getConfig().Accel, 1, 0.1, 0.5, 5)->get_gameObject(),"Changes the acceleration speed of scrolling with the joystick. Default: 1.5");
    BSML::Lite::AddHoverHint(AddConfigValueToggle(vertical, getConfig().IsLinear)->get_gameObject(),"Toggles whether you want to use acceleration or not.");
}

extern "C" void load() {
  il2cpp_functions::Init();
  BSML::Init();
  BSML::Register::RegisterSettingsMenu("Faster Scroll", DidActivate, false);
  INSTALL_HOOK(Logger, LevelCollectionTableView_OnEnable);
  INSTALL_HOOK(Logger, ScrollView_Awake);
  INSTALL_HOOK(Logger, ScrollView_HandleJoystickWasNotCenteredThisFrame);
  INSTALL_HOOK(Logger, ScrollView_HandleJoystickWasCenteredThisFrame);
  custom_types::Register::AutoRegister();
}
