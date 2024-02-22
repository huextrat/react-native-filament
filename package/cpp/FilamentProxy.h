//
// Created by Marc Rousavy on 20.02.24.
//

#pragma once

#include <jsi/jsi.h>

#include <string>
#include <vector>

#include "FilamentView.h"
#include "jsi/HybridObject.h"
#include "test/TestHybridObject.h"

namespace margelo {

using namespace facebook;

class FilamentProxy : public HybridObject {
private:
  virtual int loadModel(std::string path) = 0;
  virtual std::shared_ptr<FilamentView> findFilamentView(int id) = 0;

  std::shared_ptr<TestHybridObject> createTestObject();

public:
  void loadHybridMethods() override;
};

} // namespace margelo
