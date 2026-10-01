#include "app/SceneLoader.h"

#include <cstddef>
#include <fstream>
#include <nlohmann/json.hpp>
#include <numbers>
#include <stdexcept>
#include <string>

#include "core/Matrix.h"

namespace raster {
namespace {

using Json = nlohmann::json;

const char* const cObjectsKey = "objects";

[[noreturn]] void Fail(const std::string& message) {
  throw std::runtime_error("Invalid scene JSON: " + message);
}

Vec3 ReadVec3(const Json& node, const std::string& field) {
  if (!node.is_array() || node.size() != 3) {
    Fail("'" + field + "' must be an array of 3 numbers");
  }

  for (const Json& value : node) {
    if (!value.is_number()) {
      Fail("'" + field + "' must contain only numbers");
    }
  }

  return Vec3{node[0].get<double>(), node[1].get<double>(),
              node[2].get<double>()};
}

Vec3 ReadVec3Or(const Json& parent, const std::string& key,
                const Vec3& fallback) {
  if (!parent.contains(key)) {
    return fallback;
  }

  return ReadVec3(parent.at(key), key);
}

Color ReadColor(const Json& node, const std::string& field) {
  const Vec3 cValues = ReadVec3(node, field);
  return Color{cValues.x, cValues.y, cValues.z};
}

Mat4 ReadModelMatrix(const Json& object) {
  const Vec3 cTranslation =
      ReadVec3Or(object, "translate", Vec3{0.0, 0.0, 0.0});

  const Vec3 cRotationDegrees =
      ReadVec3Or(object, "rotate_degrees", Vec3{0.0, 0.0, 0.0});

  const Vec3 cScale = ReadVec3Or(object, "scale", Vec3{1.0, 1.0, 1.0});

  const double cToRadians = std::numbers::pi / 180.0;

  return MakeTranslateMatrix(cTranslation.x, cTranslation.y, cTranslation.z) *
         MakeRotateZMatrix(cRotationDegrees.z * cToRadians) *
         MakeRotateYMatrix(cRotationDegrees.y * cToRadians) *
         MakeRotateXMatrix(cRotationDegrees.x * cToRadians) *
         MakeScaleMatrix(cScale.x, cScale.y, cScale.z);
}

Camera ReadCamera(const Json& root) {
  Camera camera;

  if (!root.contains("camera")) {
    return camera;
  }

  const Json& node = root.at("camera");

  if (node.contains("fov_y_degrees")) {
    camera.fov_y_degrees = node.at("fov_y_degrees").get<double>();
  }

  if (node.contains("near")) {
    camera.near_plane = node.at("near").get<double>();
  }

  if (node.contains("far")) {
    camera.far_plane = node.at("far").get<double>();
  }

  if (node.contains("position") || node.contains("look_at") ||
      node.contains("up")) {
    const Vec3 cEye = ReadVec3Or(node, "position", Vec3{0.0, 0.0, 0.0});
    const Vec3 cCenter = ReadVec3Or(node, "look_at", Vec3{0.0, 0.0, -1.0});
    const Vec3 cUp = ReadVec3Or(node, "up", Vec3{0.0, 1.0, 0.0});

    camera.view = MakeLookAtMatrix(cEye, cCenter, cUp);
  }

  return camera;
}

Drawable ReadDrawable(const Json& object, std::size_t index) {
  if (!object.is_object()) {
    Fail(std::string{"'"} + cObjectsKey + "[" + std::to_string(index) +
         "]' must be an object");
  }

  if (!object.contains("mesh")) {
    Fail(std::string{"'"} + cObjectsKey + "[" + std::to_string(index) +
         "].mesh' is required");
  }

  Drawable drawable;
  drawable.mesh_path = object.at("mesh").get<std::string>();

  if (object.contains("texture")) {
    drawable.texture_path = object.at("texture").get<std::string>();
  }

  drawable.model = ReadModelMatrix(object);

  return drawable;
}

}  // namespace

Scene LoadSceneFromJson(const std::string& path) {
  std::ifstream input(path);

  if (!input) {
    throw std::runtime_error("Failed to open scene file: " + path);
  }

  Json root;
  try {
    root = Json::parse(input);
  } catch (const Json::parse_error& error) {
    throw std::runtime_error("Failed to parse scene file " + path + ": " +
                             error.what());
  }

  if (!root.is_object()) {
    Fail("the document root must be an object");
  }

  Scene scene;

  if (root.contains("width")) {
    scene.width = root.at("width").get<int>();
  }

  if (root.contains("height")) {
    scene.height = root.at("height").get<int>();
  }

  if (scene.width <= 0 || scene.height <= 0) {
    Fail("'width' and 'height' must be positive");
  }

  if (root.contains("background")) {
    scene.background = ReadColor(root.at("background"), "background");
  }

  if (root.contains("cull_back_faces")) {
    scene.cull_back_faces = root.at("cull_back_faces").get<bool>();
  }

  if (root.contains("output")) {
    scene.output_path = root.at("output").get<std::string>();
  }

  scene.camera = ReadCamera(root);

  if (!root.contains(cObjectsKey) || !root.at(cObjectsKey).is_array() ||
      root.at(cObjectsKey).empty()) {
    Fail("'" + std::string(cObjectsKey) +
         "' must be a non-empty array of objects");
  }

  for (std::size_t i = 0; i < root.at(cObjectsKey).size(); ++i) {
    scene.objects.push_back(ReadDrawable(root.at(cObjectsKey).at(i), i));
  }

  return scene;
}

}  // namespace raster
