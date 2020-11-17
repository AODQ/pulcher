#pragma once

#include <vector>

namespace plugin::config {

  namespace volnias::primary {
    float & ChargeupPreBeginThreshold();
    float & ChargeupBeginThreshold();
    float & ChargeupDelta();
    float & ChargeupTimerEnd();
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileForce();
    float & ProjectileDamage();
    float & Knockback();
  }

  namespace volnias::secondary {
    uint8_t & MaxChargedShots();
    float & ChargeupDelta();
    float & ChargeupMaxThreshold();
    float & ChargeupTimerStart();
    float & DischargeDelta();

    float & DischargeCooldown();
  };

  namespace grannibal::primary {
    float & MuzzleTrailTimer();
    uint32_t & MuzzleTrailParticles();

    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  };

  namespace grannibal::secondary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();

    uint32_t & Bounces();
    float & ProjectileVelocityFriction();
  }

  namespace dopplerBeam::primary {
    float & ProjectileVelocity();
    float & ProjectileForce();
    float & ProjectileDamage();
    float & DischargeCooldown();
  }

  namespace dopplerBeam::secondary {
    float & ProjectileVelocity();
    float & ProjectileForce();
    float & ProjectileDamage();
    std::vector<float> & ShotPattern();
    float & DischargeCooldown();
  };

  namespace pericaliya::primary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  };

  namespace pericaliya::secondary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();

    float & RedirectionMinimumThreshold();

    std::vector<float> & ShotPattern();
  }

  namespace zeusStinger::primary {
    float & DischargeCooldown();
    float & ProjectileForce();
    float & ProjectileDamage();
  };

  namespace zeusStinger::secondary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileVelocityFriction();
    float & ProjectileLifetime();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();

    float & RedirectionMinimumThreshold();

    std::vector<float> & ShotPattern();
  }

  namespace badFetus::primary {
    float & DischargeCooldown();
    float & ProjectileForce();
    float & ProjectileCooldown();
    float & ProjectileDamage();
  };

  namespace badFetus::secondary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileVelocityFriction();
    float & ProjectileLifetime();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  }

  namespace badFetus::combo {
    float & VelocityFriction();
    float & ExplosionRadius();
    float & ExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  }

  namespace manshredder::primary {
    float & DischargeCooldown();
    float & ProjectileForce();
    float & ProjectileCooldown();
    float & ProjectileDamage();
    float & ProjectileDistance();
  };

  namespace manshredder::secondary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  };

  namespace wallbanger::primary {
    float & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileForce();
    float & ProjectileSplashDamageMin();
    float & ProjectileSplashDamageMax();
    float & ProjectileDirectDamage();
  };

  namespace wallbanger::secondary {
    float & DischargeCooldown();
    float & ProjectileForce();
    float & ProjectileDamage();
  };

  namespace weapon {
    float & WeaponSwitchCooldown();
  };

  void SaveConfig();
  void LoadConfig();

  void RenderImGui();
}
