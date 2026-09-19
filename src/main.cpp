#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <chrono>
#include <cmath>

using namespace geode::prelude;

static bool s_modEnabled = true;
static bool s_xEnabled = true;
static bool s_yEnabled = true;
static bool s_ssbFixEnabled = true;
static bool s_debugLogsEnabled = true;
static bool s_showTimer = true;
static double s_yRoundingTarget = 0.1;
static double s_xRoundingTarget = 0.5;
static double s_tpsValue = 240.0;
static float s_lastPortalSpeed = -1.f;
static int s_debugCounter = 0;
static double s_globalMultiplier = 1.0;
static double s_positionDebt = 0.0;
static double s_lastUnroundedX = 0.0;
static double s_prevUpdateX = 0.0;
static bool s_hasPrevUpdateX = false;
static double s_cleanV = 0.0;

$on_mod(Loaded) {
	s_modEnabled = Mod::get()->getSettingValue<bool>("mod-enabled");
	s_xEnabled = Mod::get()->getSettingValue<bool>("x-enabled");
	s_yEnabled = Mod::get()->getSettingValue<bool>("y-enabled");
	s_ssbFixEnabled = Mod::get()->getSettingValue<bool>("ssb-fix-enabled");
	s_debugLogsEnabled = Mod::get()->getSettingValue<bool>("enable-debug-logs");
	s_showTimer = Mod::get()->getSettingValue<bool>("show-timer");
	s_yRoundingTarget = Mod::get()->getSettingValue<double>("y-rounding-target");
	s_xRoundingTarget = Mod::get()->getSettingValue<double>("x-rounding-target");
	s_tpsValue = Mod::get()->getSettingValue<double>("tps-value");

	listenForSettingChanges<bool>("mod-enabled", [](bool val) { s_modEnabled = val; });
	listenForSettingChanges<bool>("x-enabled", [](bool val) { s_xEnabled = val; });
	listenForSettingChanges<bool>("y-enabled", [](bool val) { s_yEnabled = val; });
	listenForSettingChanges<bool>("ssb-fix-enabled", [](bool val) { s_ssbFixEnabled = val; });
	listenForSettingChanges<bool>("enable-debug-logs", [](bool val) { s_debugLogsEnabled = val; });
	listenForSettingChanges<bool>("show-timer", [](bool val) { s_showTimer = val; });
	listenForSettingChanges<double>(
		"y-rounding-target", [](double val) { s_yRoundingTarget = val; });
	listenForSettingChanges<double>(
		"x-rounding-target", [](double val) { s_xRoundingTarget = val; });
	listenForSettingChanges<double>("tps-value", [](double val) { s_tpsValue = val; });
}

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) return false;

		static bool s_hasShownSilicateWarning = false;
		if (!s_hasShownSilicateWarning) {
			s_hasShownSilicateWarning = true;
			Loader::get()->queueInMainThread([] {
				if (Loader::get()->isModLoaded("peony.silicate")) {
					FLAlertLayer::create("Warning!",
						"Position Rounding detected Silicate mod, this mod is supposed to work "
						"with Silicate but inorder for SSB fix to work you will have to go to "
						"Silicate's menu and find settings tab, and then disable the mod and "
						"reenable it, while NOT being in a level.",
						"OK")
						->show();
				}
			});
		}

		return true;
	}
};


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

class $modify(MyScheduler, CCScheduler) {
	void update(float dt) {
		if (s_modEnabled && s_xEnabled && s_ssbFixEnabled) {
			dt *= static_cast<float>(s_globalMultiplier);
		}
		CCScheduler::update(dt);
	}
};

class $modify(TimerPlayLayer, PlayLayer) {
	struct Fields {
		CCLabelTTF* m_timerLabel = nullptr;
		std::chrono::high_resolution_clock::time_point m_startTime;
		bool m_isTimerRunning = false;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		auto winSize = CCDirector::get()->getWinSize();

		m_fields->m_timerLabel = CCLabelTTF::create("0.000s", "Arial", 24.0f);
		m_fields->m_timerLabel->setPosition(ccp(50, winSize.height - 25));
		m_fields->m_timerLabel->setAnchorPoint({0.f, 0.5f});
		m_fields->m_timerLabel->setColor({255, 255, 255});
		m_fields->m_timerLabel->setZOrder(999);

		m_fields->m_timerLabel->setVisible(s_showTimer && s_modEnabled);
		this->addChild(m_fields->m_timerLabel);

		m_fields->m_isTimerRunning = false;

		this->getScheduler()->scheduleSelector(
			schedule_selector(TimerPlayLayer::updateTimer), this, 0.0f, false);

		return true;
	}

	void updateTimer(float dt) {
		if (m_fields->m_timerLabel) {
			m_fields->m_timerLabel->setVisible(s_showTimer && s_modEnabled);
		}

		if (s_showTimer && s_modEnabled && m_fields->m_isTimerRunning && m_fields->m_timerLabel) {
			auto now = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsed = now - m_fields->m_startTime;
			float realTime = elapsed.count();

			std::string timeStr = fmt::format("{:.3f}s", realTime);
			m_fields->m_timerLabel->setString(timeStr.c_str());
		}
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		s_positionDebt = 0.0;
		s_globalMultiplier = 1.0;
		s_lastUnroundedX = 0.0;
		s_prevUpdateX = 0.0;
		s_hasPrevUpdateX = false;
		s_cleanV = 0.0;

		m_fields->m_startTime = std::chrono::high_resolution_clock::now();
		m_fields->m_isTimerRunning = true;

		if (m_fields->m_timerLabel) {
			m_fields->m_timerLabel->setString("0.000s");
		}
	}

	void destroyPlayer(PlayerObject* player, GameObject* object) {
		PlayLayer::destroyPlayer(player, object);

		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed = now - m_fields->m_startTime;
		float realTime = elapsed.count();

		if (m_fields->m_isTimerRunning && realTime > 0.1f) {
			m_fields->m_isTimerRunning = false;
			if (s_debugLogsEnabled) {
				log::info("[Timer] Player died! Final real time: {:.3f}s", realTime);
			}
		}
	}
};

class $modify(PlayerObject) {
	static const int PRIORITY = 100;
	void setYVelocity(double velocity, int type) {
		if (s_modEnabled && s_yEnabled && s_yRoundingTarget > 0.0) {
			velocity = std::round(velocity / s_yRoundingTarget) * s_yRoundingTarget;
		}
		PlayerObject::setYVelocity(velocity, type);
	}

	void setPosition(CCPoint const& pos) {
		auto pl = PlayLayer::get();
		if (!pl || this != pl->m_player1) {
			PlayerObject::setPosition(pos);
			return;
		}

		CCPoint newPos = pos;
		if (s_modEnabled && s_xEnabled && s_xRoundingTarget > 0.0) {
			if (s_xRoundingTarget < 5.0) {
				double originalX = newPos.x;

				s_lastUnroundedX = originalX;

				newPos.x = std::round(originalX / s_xRoundingTarget) * s_xRoundingTarget;
				s_positionDebt += (originalX - newPos.x);
			}
		}
		PlayerObject::setPosition(newPos);
	}

	void update(float dt) {
		auto pl = PlayLayer::get();
		bool isMainPlayer = (pl && this == pl->m_player1);

		if (isMainPlayer && s_modEnabled && s_xEnabled && s_xRoundingTarget > 0.0 &&
			s_tpsValue > 0.0) {
			constexpr double lerpK = 0.25;

			if (s_hasPrevUpdateX) {
				double deltaX = s_lastUnroundedX - s_prevUpdateX;
				s_cleanV = deltaX * s_tpsValue;
			}
			s_prevUpdateX = s_lastUnroundedX;
			s_hasPrevUpdateX = true;

			if (std::abs(s_cleanV) > 1.0) {
				double targetMultiplier = 1.0 +
					((s_positionDebt * s_tpsValue / s_cleanV) - (s_globalMultiplier - 1.0)) / lerpK;

				s_globalMultiplier += (targetMultiplier - s_globalMultiplier) * lerpK;

				double actualDv = (s_globalMultiplier - 1.0) * s_cleanV;
				double compensated = actualDv / s_tpsValue;
				s_positionDebt -= compensated;
			}

			s_globalMultiplier = std::clamp(s_globalMultiplier, 0.3, 2.0);
			s_globalMultiplier = std::round(s_globalMultiplier * 100000.0) / 100000.0;
		} else if (isMainPlayer) {
			s_globalMultiplier = 1.0;
			s_positionDebt = 0.0;
			s_hasPrevUpdateX = false;
		}

		PlayerObject::update(dt);

		if (!isMainPlayer || !s_modEnabled || !s_xEnabled || !s_ssbFixEnabled) {
			return;
		}

		float currentSpeed = this->m_playerSpeed;
		if (currentSpeed != s_lastPortalSpeed || s_debugCounter == 0) {
			s_lastPortalSpeed = currentSpeed;
			s_debugCounter = 15;
		}

		if (s_debugLogsEnabled && s_debugCounter > 0) {
			log::info("Target: {:.2f} | Debt: {:.3f}, CleanV: {:.2f}, Multiplier: {:.5f}",
				s_xRoundingTarget, s_positionDebt, s_cleanV, s_globalMultiplier);
			s_debugCounter--;
		}
	}
};