/*
 * TÖVE - Animated vector graphics for LÖVE.
 * https://github.com/poke1024/tove2d
 *
 * Copyright (c) 2018, Bernhard Liebl
 *
 * Distributed under the MIT license. See LICENSE file for details.
 *
 * All rights reserved.
 */

#include "../common.h"

BEGIN_TOVE_NAMESPACE

class Segment {
private:
	ClipperPoint _begin;
	ClipperPoint _end;
	float _length;
	float _dir[2];

public:
	Segment() {
	}

	Segment &operator=(const Segment &s) {
		_begin = s._begin;
		_end = s._end;
		_length = s._length;
		_dir[0] = s._dir[0];
		_dir[1] = s._dir[1];
		return *this;
	}

	Segment(const ClipperPoint &begin, const ClipperPoint &end) : _begin(begin), _end(end) {
		int dx = end.X - begin.X;
		int dy = end.Y - begin.Y;
		_length = std::sqrt(dx * dx + dy * dy);
		_dir[0] = dx / _length;
		_dir[1] = dy / _length;
	}

	inline float length() const {
		return _length;
	}

	inline ClipperPoint at(float t) const {
		return ClipperPoint(_begin.X + _dir[0] * t, _begin.Y + _dir[1] * t);
	}

	inline const ClipperPoint &begin() const {
		return _begin;
	}

	inline const ClipperPoint &end() const {
		return _end;
	}
};

class Segments {
private:
	std::vector<Segment> _segments;
	int _next;

public:
	Segments(const ClipperPath &points, float offset) : _next(0) {
		std::vector<Segment> segments;
		float length = 0;
		for (int i = 0; i < points.size(); i++) {
			Segment s(points[i], points[(i + 1) % points.size()]);
			segments.push_back(s);
			length += s.length();
		}

		if (offset == 0.0) {
			_segments = std::move(segments);
		} else {
			offset = fmod(offset, length);
			if (offset < 0.0) {
				offset += length;
			}

			int i = 0;
			while (offset > segments[i].length()) {
				offset -= segments[i].length();
				i = (i + 1) % segments.size();
			}

			std::vector<Segment> shifted;
			shifted.push_back(Segment(segments[i].at(offset), segments[i].end()));
			int j = (i + 1) % segments.size();
			while (j != i) {
				shifted.push_back(segments[j]);
				j = (j + 1) % segments.size();
			}
			shifted.push_back(Segment(segments[i].begin(), segments[i].at(offset)));

			_segments = std::move(shifted);
		}
	}

	Segment pop() {
		return _segments[_next++];
	}

	bool empty() const {
		return _next == _segments.size();
	}
};

class Turtle {
private:
	Segments _segments;
	Segment _current;
	ClipperPoint _begin;
	float _t;
	bool _down;
	ClipperPaths &_out;

	void draw(const ClipperPoint &a, const ClipperPoint &b) {
		ClipperPath path;
		path.resize(2);
		path[0] = a;
		path[1] = b;
		_out.push_back(path);
	}

public:
	Turtle(const ClipperPath &path, float offset, ClipperPaths &out) :
		_segments(path, offset), _t(0), _down(true), _out(out) {
		_current = _segments.pop();
		_begin = _current.begin();
	}

	bool push(float d) {
		while (d > _current.length() - _t) {
			if (_down) {
				draw(_begin, _current.end());
			}

			if (_segments.empty()) {
				return false;
			}

			d -= _current.length() - _t;
			_current = _segments.pop();
			if (_down) {
				_begin = _current.begin();
			}

			_t = 0;
		}

		_t += d;
		return true;
	}

	void toggle() {
		if (_down) {
			draw(_begin, _current.at(_t));
		}

		_down = !_down;

		if (_down) {
			_begin = _current.at(_t);
		}
	}
};

END_TOVE_NAMESPACE
