#pragma once

#include <luse.h>
#include <parametricCurveCollection.h>

namespace rpcore {

/**
 * Interface to a curve which also manages connecting the end of the
 * curve with the beginning.
 */
class SmoothConnectedCurve
{
public:
	SmoothConnectedCurve(void);

	/** Returns a list of all controll points. */
	const std::vector<LVecBase2f>& get_control_points(void) const;

	/** Sets the cv points to the given list of points. */
	void set_control_points(const std::vector<LVecBase2f>& points);

	/** Returns the display color of the curve. */
	const LVecBase3f& get_color(void) const;

	/** Sets the display color of the curve. */
	void set_color(const LVecBase3f& rgb);

	/** Sets the curve to be linear, and only use a single value. */
	void set_single_value(float val);

	/** Appends a new cv and returns the index of the attached cv. */
	size_t append_cv(float x, float y);

	/**
	 * Attempts to remove the cv at the given index, does nothing if only
	 * one control point is left.
	 */
	void remove_cv(size_t index);

	/** Rebuilds the curve based on the controll point values. */
	void build_curve(void);

	/** Updates the cv point at the given index. */
	void set_cv_value(size_t index, float x_value, float y_value);

	/**
	 * Returns the value on the curve ranging whereas the offset should be
	 * from 0 to 1 (0 denotes the start of the curve). The returned value will
	 * be a value from 0 to 1 as well.
	 */
	float get_value(float offset) const;

	/** Returns the value of the curve as yaml list. */
	std::string serialize(void) const;

private:
	bool _modified = false;
	int _border_points = 1;
	LVecBase3f _color = LVecBase3f(0, 0, 0);
	std::vector<LVecBase2f> _cv_points;
	PT(ParametricCurveCollection) _curve;
};

// ************************************************************************************************
inline const std::vector<LVecBase2f>& SmoothConnectedCurve::get_control_points(void) const
{
	return _cv_points;
}

inline void SmoothConnectedCurve::set_control_points(const std::vector<LVecBase2f>& points)
{
	_cv_points = points;
	_modified = true;
	build_curve();
}

inline const LVecBase3f& SmoothConnectedCurve::get_color(void) const
{
	return _color;
}

inline void SmoothConnectedCurve::set_color(const LVecBase3f& rgb)
{
	_color = rgb;
}

inline void SmoothConnectedCurve::set_single_value(float val)
{
	_modified = false;
	_cv_points = { {0.5f, val} };
	build_curve();
}

inline size_t SmoothConnectedCurve::append_cv(float x, float y)
{
	_cv_points.push_back({x, y});
	build_curve();
	_modified = true;
	return _cv_points.size() - 1;
}

inline void SmoothConnectedCurve::remove_cv(size_t index)
{
	if (_cv_points.size() > 1)
		_cv_points.erase(_cv_points.begin() + index);
	_modified = true;
	build_curve();
}

inline void SmoothConnectedCurve::set_cv_value(size_t index, float x_value, float y_value)
{
	_cv_points[index] = LVecBase2f(x_value, y_value);
	_modified = true;
}

inline float SmoothConnectedCurve::get_value(float offset) const
{
	LVecBase3f point(0);
	_curve->evaluate_xyz(offset, point);
	const float y = point.get_y();
	return max(0.0f, min(1.0f, y));
}

}