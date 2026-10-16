#pragma once
#include "StageData.h"
#include <algorithm>

class CameraSectionState
{
  public:

    // 更新処理
    bool Update(const std::vector<CameraLimitZoneData>& zones, float x, float y, bool reset = false)
    {
        const auto contains = [x, y](const CameraLimitZoneData& zone)
        {
            return x >= zone.minX && x <= zone.maxX && y >= zone.minY && y <= zone.maxY;
        };
        const auto exists = [&zones](const std::string& section)
        {
            return std::any_of(zones.begin(), zones.end(), [&section](const auto& zone)
                               { return !zone.isSectionTrigger && zone.cameraSection == section; });
        };
        const std::string previous = activeSection;
        const bool wasInitialized = initialized && !reset;

        if (!wasInitialized || !exists(activeSection))
        {
            initialized = false;

            for (const auto& zone : zones)
            {
                if (!zone.isSectionTrigger && contains(zone))
                {
                    activeSection = zone.cameraSection;
                    initialized = true;
                    break;
                }
            }

            if (!initialized)
            {
                for (const auto& zone : zones)
                {
                    if (!zone.isSectionTrigger)
                    {
                        activeSection = zone.cameraSection;
                        initialized = true;
                        break;
                    }
                }
            }
        }

        const bool currentTrigger = std::any_of(
            zones.begin(), zones.end(), [&](const auto& zone)
            { return zone.isSectionTrigger && zone.cameraSection == activeSection && contains(zone); });

        if (!currentTrigger)
        {
            for (const auto& zone : zones)
            {
                if (zone.isSectionTrigger && contains(zone) && exists(zone.cameraSection))
                {
                    activeSection = zone.cameraSection;
                    initialized = true;
                    break;
                }
            }
        }

        return wasInitialized && previous != activeSection;
    }

    std::vector<CameraLimitZoneData> GetBounds(const std::vector<CameraLimitZoneData>& zones) const
    {
        std::vector<CameraLimitZoneData> result;

        for (const auto& zone : zones)
        {
            if (!zone.isSectionTrigger && zone.cameraSection == activeSection)
            {
                result.push_back(zone);
            }
        }

        return result;
    }

    const std::string& GetSection() const
    {
        return activeSection;
    }

  private:

    std::string activeSection;
    bool initialized = false;
};
