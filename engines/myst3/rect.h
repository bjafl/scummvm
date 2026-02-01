
#ifndef RECT_H_
#define RECT_H_
#include "common/rect.h"

namespace Myst3 {
// BEGIN_POINT_TYPE(float, PointF);
// END_POINT_TYPE(float, PointF);

struct PointF : public Common::PointBase<float, PointF> {
	constexpr PointF() : PointBase() {}
	constexpr PointF(float x, float y) : PointBase(x, y) {}
	constexpr PointF(const Common::Point &p) : PointBase(p.x, p.y) {}
};
static inline PointF operator*(int multiplier, const PointF &p) { return PointF(p.x * multiplier, p.y * multiplier); }
static inline PointF operator*(double multiplier, const PointF &p) { return PointF((float)(p.x * multiplier), (float)(p.y * multiplier)); }
static inline PointF operator*(Common::Point point, const PointF &p) { return PointF((float)(p.x * point.x), (float)(p.y * point.y)); }
static inline PointF operator*(PointF point, const PointF &p) { return PointF((p.x * point.x), (p.y * point.y)); }

struct Point : public Common::PointBase<int16, Point> {
	constexpr Point() : PointBase() {}
	constexpr Point(float x, float y) : PointBase(x, y) {}
	constexpr Point(const Common::Point &p) : PointBase(p.x, p.y) {}
	constexpr operator Common::Point() const {
		return Common::Point(x, y);
	}
};
static inline Point operator*(int multiplier, const Point &p) { return Point(p.x * multiplier, p.y * multiplier); }
static inline Point operator*(double multiplier, const Point &p) { return Point((int16)(p.x * multiplier), (int16)(p.y * multiplier)); }
static inline Point operator*(Common::Point point, const Point &p) { return Point((int16)(p.x * point.x), (int16)(p.y * point.y)); }
static inline Point operator*(PointF pointF, const Point &p) { return Point(p.x * pointF.x, p.y * pointF.y); }

struct Rect : public Common::RectBase<int16, Rect, Point> {
	constexpr Rect() : RectBase() {}
	constexpr Rect(int16 w, int16 h) : RectBase(w, h) {}
	Rect(const Point &topLeft, const Point &bottomRight) : RectBase(topLeft, bottomRight) {}
	constexpr Rect(const Point &topLeft, int16 w, int16 h) : RectBase(topLeft, w, h) {}
	Rect(int16 x1, int16 y1, int16 x2, int16 y2) : RectBase(x1, y1, x2, y2) {}
	operator Common::Rect() const {
		return Common::Rect(left, top, right, bottom);
	}
	Rect centerIn(const Rect &rect) const {
		Rect r(rect);
		r.translate((width() - rect.width()) / 2,
					(height() - rect.height()) / 2);
		return r;
	}
	Rect fitInside(const Rect &rect, bool center = true) const {
		float aspectRatio = width() / (float)height();
		int16 w = rect.width();
		int16 h = rect.height();
		int16 newW = MIN<int16>(w, h * aspectRatio);
		int16 newH = MIN<int16>(h, w / aspectRatio);
		Rect r(w, h);
		r.translate(left, top);
		if (center) {
			r.translate((w - newW) / 2, (h - newH) / 2);
		}
		return r;
	}
};
static inline Rect operator*(const Rect &r, const PointF &pointF) {
	Rect r2(r);
	r2.setWidth(r.width() * pointF.x);
	r2.setHeight(r.height() * pointF.y);
	return r2;
}
} // namespace Myst3

#endif // RECT_H_