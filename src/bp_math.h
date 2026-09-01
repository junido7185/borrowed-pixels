#pragma once

#include <cstdint>

namespace bp {

inline constexpr std::int32_t kQ8One = 256;
inline constexpr std::int32_t kInputUnit = 32767;

struct Vec2Q8 {
    std::int32_t x;
    std::int32_t y;
};

struct RectPixels {
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
};

struct CollisionResolution {
    std::int32_t shiftXQ8;
    std::int32_t shiftYQ8;
    bool collided;
};

constexpr std::int32_t Clamp32(
    std::int32_t value,
    std::int32_t minimum,
    std::int32_t maximum) noexcept {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr std::int32_t Sign32(std::int32_t value) noexcept {
    return value < 0 ? -1 : (value > 0 ? 1 : 0);
}

constexpr std::int32_t RoundDivideSigned(std::int64_t numerator, std::int32_t denominator) noexcept {
    if (denominator <= 0) {
        return 0;
    }
    const std::int64_t half = denominator / 2;
    return numerator < 0
        ? static_cast<std::int32_t>(-((-numerator + half) / denominator))
        : static_cast<std::int32_t>((numerator + half) / denominator);
}

constexpr std::int32_t DivideAwayFromZero(std::int64_t numerator, std::int32_t denominator) noexcept {
    if (denominator <= 0 || numerator == 0) {
        return 0;
    }
    return numerator < 0
        ? static_cast<std::int32_t>(-((-numerator + denominator - 1) / denominator))
        : static_cast<std::int32_t>((numerator + denominator - 1) / denominator);
}

constexpr std::int32_t TruncMultiplyDivide(
    std::int32_t value,
    std::int32_t multiplier,
    std::int32_t denominator) noexcept {
    if (denominator <= 0) {
        return 0;
    }
    return static_cast<std::int32_t>(
        (static_cast<std::int64_t>(value) * multiplier) / denominator);
}

constexpr std::int32_t PixelsPerSecondToQ8PerTick(std::int32_t pixelsPerSecond) noexcept {
    const std::int64_t magnitude = pixelsPerSecond < 0
        ? -static_cast<std::int64_t>(pixelsPerSecond)
        : pixelsPerSecond;
    const std::int32_t converted = static_cast<std::int32_t>((magnitude * kQ8One + 30) / 60);
    return pixelsPerSecond < 0 ? -converted : converted;
}

constexpr std::int32_t PixelsPerSecondSquaredToQ8PerTickSquared(
    std::int32_t pixelsPerSecondSquared) noexcept {
    const std::int64_t magnitude = pixelsPerSecondSquared < 0
        ? -static_cast<std::int64_t>(pixelsPerSecondSquared)
        : pixelsPerSecondSquared;
    const std::int32_t converted = static_cast<std::int32_t>((magnitude * kQ8One + 1800) / 3600);
    return pixelsPerSecondSquared < 0 ? -converted : converted;
}

constexpr std::uint32_t IntegerSqrt(std::uint64_t value) noexcept {
    std::uint64_t result = 0;
    std::uint64_t bit = static_cast<std::uint64_t>(1) << 62;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return static_cast<std::uint32_t>(result);
}

constexpr Vec2Q8 NormalizeInput(std::int16_t x, std::int16_t y) noexcept {
    const std::int32_t inputX = x;
    const std::int32_t inputY = y;
    const std::uint64_t lengthSquared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(inputX) * inputX)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(inputY) * inputY);
    const std::uint32_t length = IntegerSqrt(lengthSquared);
    if (length == 0) {
        return Vec2Q8{0, 0};
    }
    const std::int32_t divisor = length > static_cast<std::uint32_t>(kInputUnit)
        ? static_cast<std::int32_t>(length)
        : kInputUnit;
    return Vec2Q8{
        RoundDivideSigned(static_cast<std::int64_t>(inputX) * kInputUnit, divisor),
        RoundDivideSigned(static_cast<std::int64_t>(inputY) * kInputUnit, divisor),
    };
}

constexpr void ClampVectorLength(Vec2Q8& vector, std::int32_t maximumLength) noexcept {
    if (maximumLength <= 0) {
        vector = Vec2Q8{0, 0};
        return;
    }
    const std::uint64_t lengthSquared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.x) * vector.x)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(vector.y) * vector.y);
    const std::uint64_t maximumSquared =
        static_cast<std::uint64_t>(maximumLength) * maximumLength;
    if (lengthSquared <= maximumSquared || lengthSquared == 0) {
        return;
    }
    const std::uint32_t floorLength = IntegerSqrt(lengthSquared);
    const std::uint32_t ceilingLength = floorLength
        + (static_cast<std::uint64_t>(floorLength) * floorLength < lengthSquared ? 1u : 0u);
    vector.x = static_cast<std::int32_t>(
        static_cast<std::int64_t>(vector.x) * maximumLength / ceilingLength);
    vector.y = static_cast<std::int32_t>(
        static_cast<std::int64_t>(vector.y) * maximumLength / ceilingLength);
}

constexpr std::int32_t ActorRadiusPixels(std::uint16_t shellMass) noexcept {
    return 4 + static_cast<std::int32_t>(
        IntegerSqrt(static_cast<std::uint64_t>(shellMass) * 81u) / 16u);
}

constexpr CollisionResolution ClampCircleToRect(
    Vec2Q8& center,
    std::int32_t radiusPixels,
    const RectPixels& bounds) noexcept {
    const std::int32_t radiusQ8 = radiusPixels * kQ8One;
    const std::int32_t minimumX = bounds.x * kQ8One + radiusQ8;
    const std::int32_t minimumY = bounds.y * kQ8One + radiusQ8;
    const std::int32_t maximumX = (bounds.x + bounds.width) * kQ8One - radiusQ8;
    const std::int32_t maximumY = (bounds.y + bounds.height) * kQ8One - radiusQ8;
    const Vec2Q8 before = center;
    center.x = Clamp32(center.x, minimumX, maximumX);
    center.y = Clamp32(center.y, minimumY, maximumY);
    return CollisionResolution{
        center.x - before.x,
        center.y - before.y,
        center.x != before.x || center.y != before.y,
    };
}

constexpr CollisionResolution ResolveCircleAgainstAabb(
    const Vec2Q8& center,
    std::int32_t radiusPixels,
    const RectPixels& obstacle) noexcept {
    if (radiusPixels <= 0 || obstacle.width <= 0 || obstacle.height <= 0) {
        return CollisionResolution{0, 0, false};
    }
    const std::int32_t minimumX = obstacle.x * kQ8One;
    const std::int32_t minimumY = obstacle.y * kQ8One;
    const std::int32_t maximumX = (obstacle.x + obstacle.width) * kQ8One;
    const std::int32_t maximumY = (obstacle.y + obstacle.height) * kQ8One;
    const std::int32_t radiusQ8 = radiusPixels * kQ8One;
    const std::int32_t closestX = Clamp32(center.x, minimumX, maximumX);
    const std::int32_t closestY = Clamp32(center.y, minimumY, maximumY);
    const std::int32_t deltaX = center.x - closestX;
    const std::int32_t deltaY = center.y - closestY;
    const std::uint64_t distanceSquared =
        static_cast<std::uint64_t>(static_cast<std::int64_t>(deltaX) * deltaX)
        + static_cast<std::uint64_t>(static_cast<std::int64_t>(deltaY) * deltaY);
    const std::uint64_t radiusSquared = static_cast<std::uint64_t>(radiusQ8) * radiusQ8;
    if (distanceSquared >= radiusSquared) {
        return CollisionResolution{0, 0, false};
    }

    const std::uint32_t distance = IntegerSqrt(distanceSquared);
    if (distance != 0) {
        const std::int32_t penetration = radiusQ8 - static_cast<std::int32_t>(distance);
        return CollisionResolution{
            DivideAwayFromZero(static_cast<std::int64_t>(deltaX) * penetration, static_cast<std::int32_t>(distance)),
            DivideAwayFromZero(static_cast<std::int64_t>(deltaY) * penetration, static_cast<std::int32_t>(distance)),
            true,
        };
    }

    const bool inside = center.x > minimumX && center.x < maximumX
        && center.y > minimumY && center.y < maximumY;
    if (!inside) {
        if (center.x <= minimumX) return CollisionResolution{-radiusQ8, 0, true};
        if (center.x >= maximumX) return CollisionResolution{radiusQ8, 0, true};
        if (center.y <= minimumY) return CollisionResolution{0, -radiusQ8, true};
        return CollisionResolution{0, radiusQ8, true};
    }

    const std::int32_t left = center.x - minimumX + radiusQ8;
    const std::int32_t right = maximumX - center.x + radiusQ8;
    const std::int32_t top = center.y - minimumY + radiusQ8;
    const std::int32_t bottom = maximumY - center.y + radiusQ8;
    std::int32_t best = left;
    CollisionResolution resolution{-left, 0, true};
    if (right < best) { best = right; resolution = CollisionResolution{right, 0, true}; }
    if (top < best) { best = top; resolution = CollisionResolution{0, -top, true}; }
    if (bottom < best) { resolution = CollisionResolution{0, bottom, true}; }
    return resolution;
}

}  // namespace bp
