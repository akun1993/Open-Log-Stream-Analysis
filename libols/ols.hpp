/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@olsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/* Useful C++ classes and bindings for base ols data */

#pragma once

#include "ols.h"

/* RAII wrappers */

template<typename T, void release(T)> class OLSRefAutoRelease;
template<typename T, void addref(T), void release(T)> class OLSRef;
template<typename T, T getref(T), void release(T)> class OLSSafeRef;

using OLSObject =
	OLSSafeRef<ols_object_t *, ols_object_get_ref, ols_object_release>;
using OLSSource =
	OLSSafeRef<ols_source_t *, ols_source_get_ref, ols_source_release>;

using OLSData = OLSRef<ols_data_t *, ols_data_addref, ols_data_release>;
using OLSDataArray = OLSRef<ols_data_array_t *, ols_data_array_addref,
			    ols_data_array_release>;
using OLSOutput =
	OLSSafeRef<ols_output_t *, ols_output_get_ref, ols_output_release>;


using OLSWeakObject = OLSRef<ols_weak_object_t *, ols_weak_object_addref,
			     ols_weak_object_release>;
using OLSWeakSource = OLSRef<ols_weak_source_t *, ols_weak_source_addref,
			     ols_weak_source_release>;
using OLSWeakOutput = OLSRef<ols_weak_output_t *, ols_weak_output_addref,
			     ols_weak_output_release>;


#define OLS_AUTORELEASE
using OLSObjectAutoRelease =
	OLSRefAutoRelease<ols_object_t *, ols_object_release>;
using OLSSourceAutoRelease =
	OLSRefAutoRelease<ols_source_t *, ols_source_release>;
using OLSSceneAutoRelease = OLSRefAutoRelease<ols_scene_t *, ols_scene_release>;
using OLSDataAutoRelease = OLSRefAutoRelease<ols_data_t *, ols_data_release>;
using OLSDataArrayAutoRelease =
	OLSRefAutoRelease<ols_data_array_t *, ols_data_array_release>;
using OLSOutputAutoRelease =
	OLSRefAutoRelease<ols_output_t *, ols_output_release>;

using OLSWeakObjectAutoRelease =
	OLSRefAutoRelease<ols_weak_object_t *, ols_weak_object_release>;
using OLSWeakSourceAutoRelease =
	OLSRefAutoRelease<ols_weak_source_t *, ols_weak_source_release>;
using OLSWeakOutputAutoRelease =
	OLSRefAutoRelease<ols_weak_output_t *, ols_weak_output_release>;


template<typename T, void release(T)> class OLSRefAutoRelease {
protected:
	T val;

public:
	inline OLSRefAutoRelease() : val(nullptr) {}
	inline OLSRefAutoRelease(T val_) : val(val_) {}
	OLSRefAutoRelease(const OLSRefAutoRelease &ref) = delete;
	inline OLSRefAutoRelease(OLSRefAutoRelease &&ref) : val(ref.val)
	{
		ref.val = nullptr;
	}

	inline ~OLSRefAutoRelease() { release(val); }

	inline operator T() const { return val; }
	inline T Get() const { return val; }

	inline bool operator==(T p) const { return val == p; }
	inline bool operator!=(T p) const { return val != p; }

	inline OLSRefAutoRelease &operator=(OLSRefAutoRelease &&ref)
	{
		if (this != &ref) {
			release(val);
			val = ref.val;
			ref.val = nullptr;
		}

		return *this;
	}

	inline OLSRefAutoRelease &operator=(T new_val)
	{
		release(val);
		val = new_val;
		return *this;
	}
};

template<typename T, void addref(T), void release(T)>
class OLSRef : public OLSRefAutoRelease<T, release> {

	inline OLSRef &Replace(T valIn)
	{
		addref(valIn);
		release(this->val);
		this->val = valIn;
		return *this;
	}

	struct TakeOwnership {};
	inline OLSRef(T val_, TakeOwnership)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(val_)
	{
	}

public:
	inline OLSRef()
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(nullptr)
	{
	}
	inline OLSRef(const OLSRef &ref)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(ref.val)
	{
		addref(this->val);
	}
	inline OLSRef(T val_)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(val_)
	{
		addref(this->val);
	}

	inline OLSRef &operator=(const OLSRef &ref) { return Replace(ref.val); }
	inline OLSRef &operator=(T valIn) { return Replace(valIn); }

	friend OLSWeakObject OLSGetWeakRef(ols_object_t *object);
	friend OLSWeakSource OLSGetWeakRef(ols_source_t *source);
	friend OLSWeakOutput OLSGetWeakRef(ols_output_t *output);
	friend OLSWeakEncoder OLSGetWeakRef(ols_encoder_t *encoder);
	friend OLSWeakService OLSGetWeakRef(ols_service_t *service);
};

template<typename T, T getref(T), void release(T)>
class OLSSafeRef : public OLSRefAutoRelease<T, release> {

	inline OLSSafeRef &Replace(T valIn)
	{
		T newVal = getref(valIn);
		release(this->val);
		this->val = newVal;
		return *this;
	}

	struct TakeOwnership {};
	inline OLSSafeRef(T val_, TakeOwnership)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(val_)
	{
	}

public:
	inline OLSSafeRef()
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(nullptr)
	{
	}
	inline OLSSafeRef(const OLSSafeRef &ref)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(ref.val)
	{
		this->val = getref(ref.val);
	}
	inline OLSSafeRef(T val_)
		: OLSRefAutoRelease<T, release>::OLSRefAutoRelease(val_)
	{
		this->val = getref(this->val);
	}

	inline OLSSafeRef &operator=(const OLSSafeRef &ref)
	{
		return Replace(ref.val);
	}
	inline OLSSafeRef &operator=(T valIn) { return Replace(valIn); }

	friend OLSObject OLSGetStrongRef(ols_weak_object_t *weak);
	friend OLSSource OLSGetStrongRef(ols_weak_source_t *weak);
	friend OLSOutput OLSGetStrongRef(ols_weak_output_t *weak);
	friend OLSEncoder OLSGetStrongRef(ols_weak_encoder_t *weak);
	friend OLSService OLSGetStrongRef(ols_weak_service_t *weak);
};

inline OLSObject OLSGetStrongRef(ols_weak_object_t *weak)
{
	return {ols_weak_object_get_object(weak), OLSObject::TakeOwnership()};
}

inline OLSWeakObject OLSGetWeakRef(ols_object_t *object)
{
	return {ols_object_get_weak_object(object),
		OLSWeakObject::TakeOwnership()};
}

inline OLSSource OLSGetStrongRef(ols_weak_source_t *weak)
{
	return {ols_weak_source_get_source(weak), OLSSource::TakeOwnership()};
}

inline OLSWeakSource OLSGetWeakRef(ols_source_t *source)
{
	return {ols_source_get_weak_source(source),
		OLSWeakSource::TakeOwnership()};
}

inline OLSOutput OLSGetStrongRef(ols_weak_output_t *weak)
{
	return {ols_weak_output_get_output(weak), OLSOutput::TakeOwnership()};
}

inline OLSWeakOutput OLSGetWeakRef(ols_output_t *output)
{
	return {ols_output_get_weak_output(output),
		OLSWeakOutput::TakeOwnership()};
}

inline OLSEncoder OLSGetStrongRef(ols_weak_encoder_t *weak)
{
	return {ols_weak_encoder_get_encoder(weak),
		OLSEncoder::TakeOwnership()};
}

inline OLSWeakEncoder OLSGetWeakRef(ols_encoder_t *encoder)
{
	return {ols_encoder_get_weak_encoder(encoder),
		OLSWeakEncoder::TakeOwnership()};
}

inline OLSService OLSGetStrongRef(ols_weak_service_t *weak)
{
	return {ols_weak_service_get_service(weak),
		OLSService::TakeOwnership()};
}

inline OLSWeakService OLSGetWeakRef(ols_service_t *service)
{
	return {ols_service_get_weak_service(service),
		OLSWeakService::TakeOwnership()};
}

/* objects that are not meant to be instanced */
template<typename T, void destroy(T)> class OLSPtr {
	T obj;

public:
	inline OLSPtr() : obj(nullptr) {}
	inline OLSPtr(T obj_) : obj(obj_) {}
	inline OLSPtr(const OLSPtr &) = delete;
	inline OLSPtr(OLSPtr &&other) : obj(other.obj) { other.obj = nullptr; }

	inline ~OLSPtr() { destroy(obj); }

	inline OLSPtr &operator=(T obj_)
	{
		if (obj_ != obj)
			destroy(obj);
		obj = obj_;
		return *this;
	}
	inline OLSPtr &operator=(const OLSPtr &) = delete;
	inline OLSPtr &operator=(OLSPtr &&other)
	{
		if (obj)
			destroy(obj);
		obj = other.obj;
		other.obj = nullptr;
		return *this;
	}

	inline operator T() const { return obj; }

	inline bool operator==(T p) const { return obj == p; }
	inline bool operator!=(T p) const { return obj != p; }
};

using OLSDisplay = OLSPtr<ols_display_t *, ols_display_destroy>;
using OLSView = OLSPtr<ols_view_t *, ols_view_destroy>;
using OLSFader = OLSPtr<ols_fader_t *, ols_fader_destroy>;
using OLSVolMeter = OLSPtr<ols_volmeter_t *, ols_volmeter_destroy>;

/* signal handler connection */
class OLSSignal {
	signal_handler_t *handler;
	const char *signal;
	signal_callback_t callback;
	void *param;

public:
	inline OLSSignal()
		: handler(nullptr),
		  signal(nullptr),
		  callback(nullptr),
		  param(nullptr)
	{
	}

	inline OLSSignal(signal_handler_t *handler_, const char *signal_,
			 signal_callback_t callback_, void *param_)
		: handler(handler_),
		  signal(signal_),
		  callback(callback_),
		  param(param_)
	{
		signal_handler_connect_ref(handler, signal, callback, param);
	}

	inline void Disconnect()
	{
		signal_handler_disconnect(handler, signal, callback, param);
		handler = nullptr;
		signal = nullptr;
		callback = nullptr;
		param = nullptr;
	}

	inline ~OLSSignal() { Disconnect(); }

	inline void Connect(signal_handler_t *handler_, const char *signal_,
			    signal_callback_t callback_, void *param_)
	{
		Disconnect();

		handler = handler_;
		signal = signal_;
		callback = callback_;
		param = param_;
		signal_handler_connect_ref(handler, signal, callback, param);
	}

	OLSSignal(const OLSSignal &) = delete;
	OLSSignal(OLSSignal &&other) noexcept
		: handler(other.handler),
		  signal(other.signal),
		  callback(other.callback),
		  param(other.param)
	{
		other.handler = nullptr;
		other.signal = nullptr;
		other.callback = nullptr;
		other.param = nullptr;
	}

	OLSSignal &operator=(const OLSSignal &) = delete;
	OLSSignal &operator=(OLSSignal &&other) noexcept
	{
		Disconnect();

		handler = other.handler;
		signal = other.signal;
		callback = other.callback;
		param = other.param;

		other.handler = nullptr;
		other.signal = nullptr;
		other.callback = nullptr;
		other.param = nullptr;

		return *this;
	}
};
