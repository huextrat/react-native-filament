#pragma once

#include "FilamentAssetWrapper.h"
#include "core/utils/EntityWrapper.h"
#include "jsi/PointerHolder.h"

#include <filament/Scene.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <utils/Entity.h>

namespace margelo {
using namespace filament;

class SceneWrapper : public PointerHolder<Scene> {
public:
  explicit SceneWrapper(std::shared_ptr<Scene> scene) : PointerHolder("SceneWrapper", scene) {}

  void loadHybridMethods() override;

  void addAsset(std::shared_ptr<gltfio::FilamentAsset> asset);
  void removeAsset(std::shared_ptr<gltfio::FilamentAsset> asset);

private:
  std::mutex _mutex;

private: // Public JS API
  void addEntity(std::shared_ptr<EntityWrapper> entity);
  void removeEntity(std::shared_ptr<EntityWrapper> entity);
  /**
   * Removed all entities associated with the provided asset from the scene.
   */
  void removeAssetEntities(std::shared_ptr<FilamentAssetWrapper> asset);
  /**
   * Adds all entities associated with the provided asset to the scene.
   */
  void addAssetEntities(std::shared_ptr<FilamentAssetWrapper> asset);

  int getEntityCount();
};
} // namespace margelo
