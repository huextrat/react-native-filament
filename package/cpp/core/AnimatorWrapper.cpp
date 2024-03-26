//
// Created by Hanno Gödecke on 29.02.24.
//

#include "AnimatorWrapper.h"

namespace margelo {
void AnimatorWrapper::loadHybridMethods() {
  registerHybridMethod("applyAnimation", &AnimatorWrapper::applyAnimation, this);
  registerHybridMethod("updateBoneMatrices", &AnimatorWrapper::updateBoneMatrices, this);
  registerHybridMethod("applyCrossFade", &AnimatorWrapper::applyCrossFade, this);
  registerHybridMethod("resetBoneMatrices", &AnimatorWrapper::resetBoneMatrices, this);
  registerHybridMethod("getAnimationCount", &AnimatorWrapper::getAnimationCount, this);
  registerHybridMethod("getAnimationDuration", &AnimatorWrapper::getAnimationDuration, this);
  registerHybridMethod("getAnimationName", &AnimatorWrapper::getAnimationName, this);
}

Animator* AnimatorWrapper::getAnimator() {
  if (_optionalAnimator != nullptr) {
    return _optionalAnimator;
  }

  return _instance->getAnimator();
}

// TODO(copy-animations): We currently copy animations from an asset onto another instance (different model than the original asset), we
// should replace this with once we found a good solution discussed here: https://github.com/google/filament/issues/7622
AnimatorWrapper::~AnimatorWrapper() {
#ifdef HAS_FILAMENT_ANIMATOR_PATCH
  if (_optionalAnimator) {
    delete _optionalAnimator;
  }
#endif
}

inline void assertAnimationIndexSmallerThan(int animationIndex, int max) {
  if (animationIndex >= max || animationIndex < 0) {
    [[unlikely]];
    throw std::invalid_argument("Animation index out of range! Expected <" + std::to_string(max) + ", received " +
                                std::to_string(animationIndex));
  }
}

void AnimatorWrapper::applyAnimation(int animationIndex, double time) {
  Animator* animator = getAnimator();
  assertAnimationIndexSmallerThan(animationIndex, animator->getAnimationCount());
  animator->applyAnimation(animationIndex, time);
}

void AnimatorWrapper::updateBoneMatrices() {
  Animator* animator = getAnimator();
  animator->updateBoneMatrices();
}

void AnimatorWrapper::applyCrossFade(int previousAnimationIndex, double previousAnimationTime, double alpha) {
  Animator* animator = getAnimator();
  assertAnimationIndexSmallerThan(previousAnimationIndex, animator->getAnimationCount());
  animator->applyCrossFade(previousAnimationIndex, previousAnimationTime, alpha);
}

void AnimatorWrapper::resetBoneMatrices() {
  Animator* animator = getAnimator();
  animator->resetBoneMatrices();
}

int AnimatorWrapper::getAnimationCount() {
  Animator* animator = getAnimator();
  return animator->getAnimationCount();
}

double AnimatorWrapper::getAnimationDuration(int animationIndex) {
  Animator* animator = getAnimator();
  return animator->getAnimationDuration(animationIndex);
}

std::string AnimatorWrapper::getAnimationName(int animationIndex) {
  Animator* animator = getAnimator();
  return animator->getAnimationName(animationIndex);
}
} // namespace margelo
