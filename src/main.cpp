#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <cmath>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

static bool s_modEnabled = true;
static bool s_xEnabled = true;
static bool s_yEnabled = true;
static double s_yRoundingTarget = 0.1;
static double s_xRoundingTarget = 0.5;

$on_mod(Loaded) {
	s_modEnabled = Mod::get()->getSettingValue<bool>("mod-enabled");
	s_xEnabled = Mod::get()->getSettingValue<bool>("x-enabled");
	s_yEnabled = Mod::get()->getSettingValue<bool>("y-enabled");
	s_yRoundingTarget = Mod::get()->getSettingValue<double>("y-rounding-target");
	s_xRoundingTarget = Mod::get()->getSettingValue<double>("x-rounding-target");

	listenForSettingChanges<bool>("mod-enabled", [](bool val) { s_modEnabled = val; });
	listenForSettingChanges<bool>("x-enabled", [](bool val) { s_xEnabled = val; });
	listenForSettingChanges<bool>("y-enabled", [](bool val) { s_yEnabled = val; });
	listenForSettingChanges<double>(
		"y-rounding-target", [](double val) { s_yRoundingTarget = val; });
	listenForSettingChanges<double>(
		"x-rounding-target", [](double val) { s_xRoundingTarget = val; });
}

class $modify(MyPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		if (auto menu = this->getChildByID("right-button-menu")) {
			auto buttonSprite = CCSprite::create("icon.png"_spr);

			if (!buttonSprite) {
				buttonSprite = CCSprite::create("GJ_optionsBtn_001.png");
			}

			buttonSprite->setScale(0.2f);

			auto button = CCMenuItemSpriteExtra::create(
				buttonSprite, this, menu_selector(MyPauseLayer::onOpenModSettings));
			button->setID("velocity-rounding-settings-button");

			menu->addChild(button);
			menu->updateLayout();
		}
	}

	void onOpenModSettings(CCObject*) {
		openSettingsPopup(Mod::get());
	}
};

class $modify(PlayerObject) {
	void setYVelocity(double velocity, int type) {
		if (s_modEnabled && s_yEnabled && s_yRoundingTarget > 0.0) {
			velocity = std::round(velocity / s_yRoundingTarget) * s_yRoundingTarget;
		}
		PlayerObject::setYVelocity(velocity, type);
	}

	void setPosition(CCPoint const& pos) {
		CCPoint newPos = pos;
		if (s_modEnabled && s_xEnabled && s_xRoundingTarget > 0.0) {
			newPos.x = std::round(newPos.x / s_xRoundingTarget) * s_xRoundingTarget;
		}
		PlayerObject::setPosition(newPos);
	}
};