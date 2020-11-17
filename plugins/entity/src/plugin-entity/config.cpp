#include <plugin-entity/config.hpp>

#include <pulcher-gfx/imgui.hpp>
#include <pulcher-util/log.hpp>

// this file is just configs so formatting it is unnecssary

void plugin::config::RenderImGui() {
  ImGui::Begin("weapon config");

  {
    namespace config = plugin::config::volnias::primary;

    ImGui::Text("--- volnias::primary ---");
    ImGui::PushID("volnias::primary");
    ImGui::DragFloat("chargeupPreBeginThreshold", &config::ChargeupPreBeginThreshold(), 0.01f);
    ImGui::DragFloat("chargeupBeginThreshold", &config::ChargeupBeginThreshold(), 0.01f);
    ImGui::DragFloat("chargeupDelta", &config::ChargeupDelta(), 0.01f);
    ImGui::DragFloat("chargeupTimerEnd", &config::ChargeupTimerEnd(), 0.01f);
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::DragFloat("knockback", &config::Knockback(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::volnias::secondary;

    ImGui::Text("--- volnias::secondary ---");
    ImGui::PushID("volnias::secondary");
    pul::imgui::DragInt("maxChargedShots", &config::MaxChargedShots(), 0.1f);
    ImGui::DragFloat("chargeupDelta", &config::ChargeupDelta(), 0.01f);
    ImGui::DragFloat("chargeupMaxThreshold", &config::ChargeupMaxThreshold(), 0.01f);
    ImGui::DragFloat("chargeupTimerStart", &config::ChargeupTimerStart(), 0.01f);
    ImGui::DragFloat("dischargeDelta", &config::DischargeDelta(), 0.01f);
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::grannibal::primary;

    ImGui::Text("--- grannibal::primary ---");
    ImGui::PushID("grannibal::primary");
    ImGui::DragFloat("muzzleTrailTimer", &config::MuzzleTrailTimer(), 0.01f);
    pul::imgui::DragInt("muzzleTrailParticles", &config::MuzzleTrailParticles(), 0.1f);
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::grannibal::secondary;

    ImGui::Text("--- grannibal::secondary ---");
    ImGui::PushID("grannibal::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    pul::imgui::DragInt("bounces", &config::Bounces(), 0.1f);
    ImGui::DragFloat("projectileVelocityFriction", &config::ProjectileVelocityFriction(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::dopplerBeam::primary;

    ImGui::Text("--- dopplerBeam::primary ---");
    ImGui::PushID("dopplerBeam::primary");
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::dopplerBeam::secondary;

    ImGui::Text("--- dopplerBeam::secondary ---");
    ImGui::PushID("dopplerBeam::secondary");
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    for (size_t i = 0ul; i < config::ShotPattern().size(); ++ i) {
      ImGui::DragFloat(fmt::format("shot {}", i).c_str(), &config::ShotPattern()[i], 0.01f);
    }
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::pericaliya::primary;

    ImGui::Text("--- pericaliya::primary ---");
    ImGui::PushID("pericaliya::primary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::pericaliya::secondary;
    ImGui::Text("--- pericaliya::secondary ---");
    ImGui::PushID("pericaliya::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::DragFloat("redirectionMinimumThreshold", &config::RedirectionMinimumThreshold(), 0.01f);
    for (size_t i = 0ul; i < config::ShotPattern().size(); ++ i) {
      ImGui::DragFloat(fmt::format("shot {}", i).c_str(), &config::ShotPattern()[i], 0.01f);
    }
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::zeusStinger::primary;
    ImGui::Text("--- zeusStinger::primary ---");
    ImGui::PushID("zeusStinger::primary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::zeusStinger::secondary;
    ImGui::Text("--- zeusStinger::secondary ---");
    ImGui::PushID("zeusStinger::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileVelocityFriction", &config::ProjectileVelocityFriction(), 0.01f);
    ImGui::DragFloat("projectileLifetime", &config::ProjectileLifetime(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::DragFloat("redirectionMinimumThreshold", &config::RedirectionMinimumThreshold(), 0.01f);
    for (size_t i = 0ul; i < config::ShotPattern().size(); ++ i) {
      ImGui::DragFloat(fmt::format("shot {}", i).c_str(), &config::ShotPattern()[i], 0.01f);
    }
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::badFetus::primary;
    ImGui::Text("--- badFetus::primary ---");
    ImGui::PushID("badFetus::primary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileCooldown", &config::ProjectileCooldown(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::badFetus::secondary;
    ImGui::Text("--- badFetus::secondary ---");
    ImGui::PushID("badFetus::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileVelocityFriction", &config::ProjectileVelocityFriction(), 0.01f);
    ImGui::DragFloat("projectileLifetime", &config::ProjectileLifetime(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::badFetus::combo;
    ImGui::Text("--- badFetus::combo ---");
    ImGui::PushID("badFetus::combo");
    ImGui::DragFloat("velocityFriction", &config::VelocityFriction(), 0.01f);
    ImGui::DragFloat("explosionRadius", &config::ExplosionRadius(), 0.01f);
    ImGui::DragFloat("explosionForce", &config::ExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::manshredder::primary;
    ImGui::Text("--- manshredder::primary ---");
    ImGui::PushID("manshredder::primary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileCooldown", &config::ProjectileCooldown(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::DragFloat("projectileDistance", &config::ProjectileDistance(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::manshredder::secondary;
    ImGui::Text("--- manshredder::secondary ---");
    ImGui::PushID("manshredder::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileExplosionRadius", &config::ProjectileExplosionRadius(), 0.01f);
    ImGui::DragFloat("projectileExplosionForce", &config::ProjectileExplosionForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::wallbanger::primary;
    ImGui::Text("--- wallbanger::primary ---");
    ImGui::PushID("wallbanger::primary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileVelocity", &config::ProjectileVelocity(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMin", &config::ProjectileSplashDamageMin(), 0.01f);
    ImGui::DragFloat("projectileSplashDamageMax", &config::ProjectileSplashDamageMax(), 0.01f);
    ImGui::DragFloat("projectileDirectDamage", &config::ProjectileDirectDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::wallbanger::secondary;

    ImGui::Text("--- wallbanger::secondary ---");
    ImGui::PushID("wallbanger::secondary");
    ImGui::DragFloat("dischargeCooldown", &config::DischargeCooldown(), 0.01f);
    ImGui::DragFloat("projectileForce", &config::ProjectileForce(), 0.01f);
    ImGui::DragFloat("projectileDamage", &config::ProjectileDamage(), 0.01f);
    ImGui::PopID();
  }

  {
    namespace config = plugin::config::weapon;

    ImGui::Text("--- config::weapon ---");
    ImGui::PushID("config::weapon");
    ImGui::DragFloat("weaponSwitchCooldown", &config::WeaponSwitchCooldown(), 0.01f);
    ImGui::PopID();
  }

  ImGui::End();
}


//------------------------------------------------------------------------------
namespace plugin::config {
  namespace volnias::primary {
    float & ChargeupPreBeginThreshold() {
      static float chargeupPreBeginThreshold = 100.0f;
      return chargeupPreBeginThreshold;
    }

    float & ChargeupBeginThreshold() {
      static float chargeupBeginThreshold = 1000.0f;
      return chargeupBeginThreshold;
    }

    float & ChargeupDelta() {
      static float chargeupDelta = 100.0f;
      return chargeupDelta;
    }

    float & ChargeupTimerEnd() {
      static float chargeupTimerEnd = 400.0f;
      return chargeupTimerEnd;
    }

    float & DischargeCooldown() {
      static float dischargeCooldown = 300.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 10.0f;
      return projectileDamage;
    }

    float & Knockback() {
      static float knockback = 0.7f;
      return knockback;
    }

  }

  namespace volnias::secondary {
    uint8_t & MaxChargedShots() {
      static uint8_t maxChargedShots = 5;
      return maxChargedShots;
    }

    float & ChargeupDelta() {
      static float chargeupDelta = 600.0f;
      return chargeupDelta;
    }

    float & ChargeupMaxThreshold() {
      static float chargeupMaxThreshold = 6000.0f;
      return chargeupMaxThreshold;
    }

    float & ChargeupTimerStart() {
      static float chargeupTimerStart = 400.0f;
      return chargeupTimerStart;
    }

    float & DischargeDelta() {
      static float dischargeDelta = 40.0f;
      return dischargeDelta;
    }


    float & DischargeCooldown() {
      static float dischargeCooldown = 300.0f;
      return dischargeCooldown;
    }

  };

  namespace grannibal::primary {
    float & MuzzleTrailTimer() {
      static float muzzleTrailTimer = 70.0f;
      return muzzleTrailTimer;
    }

    uint32_t & MuzzleTrailParticles() {
      static uint32_t muzzleTrailParticles = 4;
      return muzzleTrailParticles;
    }

    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }

  };

  namespace grannibal::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }


    uint32_t & Bounces() {
      static uint32_t bounces = 2u;
      return bounces;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.7f;
      return projectileVelocityFriction;
    }

  }

  namespace dopplerBeam::primary {
    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 10.0f;
      return projectileDamage;
    }

    float & DischargeCooldown() {
      static float dischargeCooldown = 150.0f;
      return dischargeCooldown;
    }

  }

  namespace dopplerBeam::secondary {
    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 10.0f;
      return projectileDamage;
    }

    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.1f, 0.0f, +0.1f };
      return shotPattern;
    }

    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

  };

  namespace pericaliya::primary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }

  };

  namespace pericaliya::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }


    float & RedirectionMinimumThreshold() {
      static float redirectionMinimumThreshold = 100.0f;
      return redirectionMinimumThreshold;
    }


    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.2f, 0.0f, +0.2f };
      return shotPattern;
    }

  }

  namespace zeusStinger::primary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 80.0f;
      return projectileDamage;
    }

  };

  namespace zeusStinger::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.9f;
      return projectileVelocityFriction;
    }

    float & ProjectileLifetime() {
      static float projectileLifetime = 4000.0f;
      return projectileLifetime;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }


    float & RedirectionMinimumThreshold() {
      static float redirectionMinimumThreshold = 100.0f;
      return redirectionMinimumThreshold;
    }


    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.2f, 0.0f, +0.2f };
      return shotPattern;
    }

  }

  namespace badFetus::primary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = -0.4f;
      return projectileForce;
    }

    float & ProjectileCooldown() {
      static float projectileCooldown = 100.0f;
      return projectileCooldown;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 80.0f;
      return projectileDamage;
    }

  };

  namespace badFetus::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.9f;
      return projectileVelocityFriction;
    }

    float & ProjectileLifetime() {
      static float projectileLifetime = 4000.0f;
      return projectileLifetime;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }

  }

  namespace badFetus::combo {
    float & VelocityFriction() {
      static float velocityFriction = 0.5f;
      return velocityFriction;
    }

    float & ExplosionRadius() {
      static float explosionRadius = 64.0f;
      return explosionRadius;
    }

    float & ExplosionForce() {
      static float explosionForce = -2.0f;
      return explosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 10.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 60.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 20.0f;
      return projectileDirectDamage;
    }

  }

  namespace manshredder::primary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 80.0f;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 0.4f;
      return projectileForce;
    }

    float & ProjectileCooldown() {
      static float projectileCooldown = 80.0f;
      return projectileCooldown;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 80.0f;
      return projectileDamage;
    }

    float & ProjectileDistance() {
      static float projectileDistance = 32.0f;
      return projectileDistance;
    }

  };

  namespace manshredder::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileExplosionRadius() {
      static float projectileExplosionRadius = 96.0f;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }

  };

  namespace wallbanger::primary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    float & ProjectileSplashDamageMin() {
      static float projectileSplashDamageMin = 50.0f;
      return projectileSplashDamageMin;
    }

    float & ProjectileSplashDamageMax() {
      static float projectileSplashDamageMax = 20.0f;
      return projectileSplashDamageMax;
    }

    float & ProjectileDirectDamage() {
      static float projectileDirectDamage = 80.0f;
      return projectileDirectDamage;
    }

  };

  namespace wallbanger::secondary {
    float & DischargeCooldown() {
      static float dischargeCooldown = 1000.0f;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 0.4f;
      return projectileForce;
    }

    float & ProjectileDamage() {
      static float projectileDamage = 80.0f;
      return projectileDamage;
    }

  };

  namespace weapon {
    float & WeaponSwitchCooldown() {
      static float weaponSwitchCooldown = 300.0f;
      return weaponSwitchCooldown;
    }
  };
}
