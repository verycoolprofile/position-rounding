#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <cmath>

using namespace geode::prelude;

static bool s_modEnabled = true;
static double s_yRoundingTarget = 0.1;
static double s_xRoundingTarget = 0.5;

$on_mod(Loaded) {
	s_modEnabled = Mod::get()->getSettingValue<bool>("mod-enabled");
	s_yRoundingTarget = Mod::get()->getSettingValue<double>("y-rounding-target");
	s_xRoundingTarget = Mod::get()->getSettingValue<double>("x-rounding-target");

	listenForSettingChanges<bool>("mod-enabled", [](bool val) { s_modEnabled = val; });
	listenForSettingChanges<double>(
		"y-rounding-target", [](double val) { s_yRoundingTarget = val; });
	listenForSettingChanges<double>(
		"x-rounding-target", [](double val) { s_xRoundingTarget = val; });
}

class $modify(PlayerObject) {
	void setYVelocity(double velocity, int type) {
		if (s_modEnabled && s_yRoundingTarget > 0.0) {
			velocity = std::round(velocity / s_yRoundingTarget) * s_yRoundingTarget;
		}
		PlayerObject::setYVelocity(velocity, type);
	}

	void setPosition(CCPoint const& pos) {
		CCPoint newPos = pos;
		if (s_modEnabled && s_xRoundingTarget > 0.0) {
			newPos.x = std::round(newPos.x / s_xRoundingTarget) * s_xRoundingTarget;
		}
		PlayerObject::setPosition(newPos);
	}
};