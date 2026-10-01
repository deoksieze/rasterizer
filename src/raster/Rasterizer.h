#pragma once

#include <vector>

#include "core/Vec.h"
#include "raster/Framebuffer.h"
#include "raster/Projector.h"

namespace raster {

struct BarycentricCoordinates {
  double l1;
  double l2;
  double l3;
};

struct BoundingBox {
  int min_x;
  int max_x;
  int min_y;
  int max_y;
};

double Orientation(const Vec2& a, const Vec2& b, const Vec2& c);

bool IsFrontFacing(const ScreenTriangle& tr);
bool IsInside(const BarycentricCoordinates& bc);

BarycentricCoordinates GetBarycentricCoordinates(const ScreenTriangle& tr,
                                                 const Vec2& p);

BoundingBox FindBoundingBox(const ScreenTriangle& tr,
                            const Framebuffer& buffer);

void RasterizeTriangle(Framebuffer& buffer, const ScreenTriangle& tr);

void RasterizeTriangles(Framebuffer& buffer,
                        const std::vector<ScreenTriangle>& triangles,
                        bool cull_back_faces);

}  // namespace raster
