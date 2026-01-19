// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LABEL_SPATIAL_GRID_H
#define LABEL_SPATIAL_GRID_H

#include <cmath>
#include <vector>

#include "ui/AircraftLabel.h"

namespace viz1090 {
namespace ui {

/// Spatial hash grid for efficient neighbor queries during label physics
/// Uses a fixed cell size and maps screen coordinates to grid cells
class LabelSpatialGrid {
public:
  /// Build the grid from a list of neighbors
  /// @param neighbors The full list of label neighbors
  /// @param screenWidth Screen width for grid sizing
  /// @param screenHeight Screen height for grid sizing
  /// @param cellSize Size of each grid cell in pixels (should be >= max interaction distance)
  void build(const std::vector<LabelNeighbor>& neighbors,
             int screenWidth, int screenHeight, float cellSize = 100.0f) {
    cellSize_ = cellSize;
    invCellSize_ = 1.0f / cellSize;

    // Calculate grid dimensions (add margin for off-screen labels)
    int margin = 2;  // Extra cells on each side
    gridWidth_ = static_cast<int>(std::ceil(static_cast<float>(screenWidth) * invCellSize_)) + margin * 2;
    gridHeight_ = static_cast<int>(std::ceil(static_cast<float>(screenHeight) * invCellSize_)) + margin * 2;
    offsetX_ = margin;
    offsetY_ = margin;

    // Resize and clear grid
    int totalCells = gridWidth_ * gridHeight_;
    cells_.clear();
    cells_.resize(totalCells);

    // Insert each neighbor into appropriate cells
    // A label may span multiple cells due to its width/height
    for (size_t i = 0; i < neighbors.size(); ++i) {
      const auto& n = neighbors[i];

      // Get cell range for this label (including its bounding box)
      int minCellX = cellX(n.x);
      int maxCellX = cellX(n.x + n.w);
      int minCellY = cellY(n.y);
      int maxCellY = cellY(n.y + n.h);

      // Insert into all overlapping cells
      for (int cy = minCellY; cy <= maxCellY; ++cy) {
        for (int cx = minCellX; cx <= maxCellX; ++cx) {
          int idx = cellIndex(cx, cy);
          if (idx >= 0 && idx < totalCells) {
            cells_[idx].push_back(static_cast<uint16_t>(i));
          }
        }
      }
    }
  }

  /// Get indices of neighbors that might interact with a label at the given position
  /// @param x Label X position
  /// @param y Label Y position
  /// @param w Label width
  /// @param h Label height
  /// @param neighbors The original neighbor list (to look up by index)
  /// @param result Output vector of neighbor pointers (cleared before use)
  void getNearbyNeighbors(float x, float y, float w, float h,
                          const std::vector<LabelNeighbor>& neighbors,
                          std::vector<const LabelNeighbor*>& result) const {
    result.clear();

    // Get cell range for query (label position + size + one cell margin for interaction)
    int minCellX = cellX(x) - 1;
    int maxCellX = cellX(x + w) + 1;
    int minCellY = cellY(y) - 1;
    int maxCellY = cellY(y + h) + 1;

    // Track which neighbors we've already added (simple bitmap for small N)
    // For larger N, could use unordered_set but the overhead isn't worth it
    seenFlags_.assign(neighbors.size(), false);

    int totalCells = static_cast<int>(cells_.size());
    for (int cy = minCellY; cy <= maxCellY; ++cy) {
      for (int cx = minCellX; cx <= maxCellX; ++cx) {
        int idx = cellIndex(cx, cy);
        if (idx >= 0 && idx < totalCells) {
          for (uint16_t neighborIdx : cells_[idx]) {
            if (!seenFlags_[neighborIdx]) {
              seenFlags_[neighborIdx] = true;
              result.push_back(&neighbors[neighborIdx]);
            }
          }
        }
      }
    }
  }

private:
  /// Convert screen X to cell X
  int cellX(float screenX) const {
    return static_cast<int>(std::floor(screenX * invCellSize_)) + offsetX_;
  }

  /// Convert screen Y to cell Y
  int cellY(float screenY) const {
    return static_cast<int>(std::floor(screenY * invCellSize_)) + offsetY_;
  }

  /// Get linear index from cell coordinates
  int cellIndex(int cx, int cy) const {
    if (cx < 0 || cx >= gridWidth_ || cy < 0 || cy >= gridHeight_) {
      return -1;
    }
    return cy * gridWidth_ + cx;
  }

  float cellSize_{100.0f};
  float invCellSize_{0.01f};
  int gridWidth_{0};
  int gridHeight_{0};
  int offsetX_{0};  // Offset to handle negative coordinates
  int offsetY_{0};

  // Each cell contains indices into the neighbors vector
  std::vector<std::vector<uint16_t>> cells_;

  // Temporary flag array for deduplication (mutable for const method)
  mutable std::vector<bool> seenFlags_;
};

}  // namespace ui
}  // namespace viz1090

#endif  // LABEL_SPATIAL_GRID_H
