/*************************************************************************/
/*  webxr_interface.cpp                                                  */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifdef JAVASCRIPT_ENABLED

#include "webxr_interface.h"
#include "core/os/input.h"
#include "core/os/os.h"
#include "emscripten.h"
#include "servers/visual/visual_server_globals.h"
#include <stdlib.h>

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_supported(char *p_session_mode, bool supported) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String session_mode = String(p_session_mode);
	interface->emit_signal("session_supported", session_mode, supported);
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_started(char *p_reference_space_type) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String reference_space_type = String(p_reference_space_type);
	((WebXRInterface *)interface.ptr())->_set_reference_space_type(reference_space_type);
	interface->emit_signal("session_started");
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_ended() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	interface->uninitialize();
	interface->emit_signal("session_ended");
}

extern "C" EMSCRIPTEN_KEEPALIVE void _emwebxr_on_session_failed(char *p_message) {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL(arvr_server);

	Ref<ARVRInterface> interface = arvr_server->find_interface("WebXR");
	ERR_FAIL_COND(interface.is_null());

	String message = String(p_message);
	interface->emit_signal("session_failed", message);
}

void WebXRInterface::is_session_supported(const String &p_session_mode) {
	if (!_have_vr_support()) {
		emit_signal("session_supported", p_session_mode, false);
	} else {
		/* clang-format off */
		EM_ASM({
			const session_mode = UTF8ToString($0);
			navigator.xr.isSessionSupported(session_mode).then(function (supported) {
				ccall('_emwebxr_on_session_supported', 'void', ["string", "number"], [session_mode, supported]);
			});
		}, p_session_mode.utf8().get_data());
		/* clang-format on */
	}
}

void WebXRInterface::set_session_mode(String p_session_mode) {
	session_mode = p_session_mode;
}

String WebXRInterface::get_session_mode() const {
	return session_mode;
}

void WebXRInterface::set_required_features(String p_required_features) {
	required_features = p_required_features;
}

String WebXRInterface::get_required_features() const {
	return required_features;
}

void WebXRInterface::set_optional_features(String p_optional_features) {
	optional_features = p_optional_features;
}

String WebXRInterface::get_optional_features() const {
	return optional_features;
}

void WebXRInterface::set_requested_reference_space_types(String p_requested_reference_space_types) {
	requested_reference_space_types = p_requested_reference_space_types;
}

String WebXRInterface::get_requested_reference_space_types() const {
	return requested_reference_space_types;
}

void WebXRInterface::_set_reference_space_type(String p_reference_space_type) {
	reference_space_type = p_reference_space_type;
}

String WebXRInterface::get_reference_space_type() const {
	return reference_space_type;
}

void WebXRInterface::print_debug() const {
	/* clang-format off */
	EM_ASM({
		console.log('-- WebXRInterface debug --');
		console.log('Headset transform');
		console.log(Module['webxr_pose'].transform);
		console.log('View 1 transform');
		console.log(Module['webxr_pose'].views[0].transform.inverse);
		console.log('View 2 transform');
		console.log(Module['webxr_pose'].views[1].transform.inverse);
	});
	/* clang-format on */
}

void WebXRInterface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_session_supported", "session_mode"), &WebXRInterface::is_session_supported);
	ClassDB::bind_method(D_METHOD("set_session_mode"), &WebXRInterface::set_session_mode);
	ClassDB::bind_method(D_METHOD("get_session_mode"), &WebXRInterface::get_session_mode);
	ClassDB::bind_method(D_METHOD("set_required_features"), &WebXRInterface::set_required_features);
	ClassDB::bind_method(D_METHOD("get_required_features"), &WebXRInterface::get_required_features);
	ClassDB::bind_method(D_METHOD("set_optional_features"), &WebXRInterface::set_optional_features);
	ClassDB::bind_method(D_METHOD("get_optional_features"), &WebXRInterface::get_optional_features);
	ClassDB::bind_method(D_METHOD("get_reference_space_type"), &WebXRInterface::get_reference_space_type);
	ClassDB::bind_method(D_METHOD("set_requested_reference_space_types"), &WebXRInterface::set_requested_reference_space_types);
	ClassDB::bind_method(D_METHOD("get_requested_reference_space_types"), &WebXRInterface::get_requested_reference_space_types);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "session_mode", PROPERTY_HINT_NONE), "set_session_mode", "get_session_mode");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "required_features", PROPERTY_HINT_NONE), "set_required_features", "get_required_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "optional_features", PROPERTY_HINT_NONE), "set_optional_features", "get_optional_features");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "requested_reference_space_types", PROPERTY_HINT_NONE), "set_requested_reference_space_types", "get_requested_reference_space_types");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "reference_space_type", PROPERTY_HINT_NONE), "", "get_reference_space_type");

	ClassDB::bind_method(D_METHOD("print_debug"), &WebXRInterface::print_debug);

	ADD_SIGNAL(MethodInfo("session_supported", PropertyInfo(Variant::STRING, "session_mode"), PropertyInfo(Variant::BOOL, "supported")));
	ADD_SIGNAL(MethodInfo("session_started"));
	ADD_SIGNAL(MethodInfo("session_ended"));
	ADD_SIGNAL(MethodInfo("session_failed", PropertyInfo(Variant::STRING, "message")));
}

StringName WebXRInterface::get_name() const {
	return "WebXR";
};

int WebXRInterface::get_capabilities() const {
	return ARVRInterface::ARVR_STEREO;
};

bool WebXRInterface::is_stereo() {
	// @todo WebXR can be mono! So, how do we know? Count the views in the frame?
	return true;
};

bool WebXRInterface::is_initialized() const {
	return (initialized);
};

bool WebXRInterface::initialize() {
	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL_V(arvr_server, false);

	if (!initialized) {
		if (!_have_vr_support()) {
			return false;
		}

		if (requested_reference_space_types.size() == 0) {
			return false;
		}

		// make this our primary interface
		arvr_server->set_primary_interface(this);

		initialized = true;

		/* clang-format off */
		EM_ASM({
			// Initialize our webxr_blit_texture function.
			if (!Module.webxr_blit_texture) {
				Module.webxr_blit_texture = (function (gl) {
					function initShaderProgram(gl, vsSource, fsSource) {
						const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
						const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

						const shaderProgram = gl.createProgram();
						gl.attachShader(shaderProgram, vertexShader);
						gl.attachShader(shaderProgram, fragmentShader);
						gl.linkProgram(shaderProgram);

						if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
							alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
							return null;
						}

						return shaderProgram;
					}

					function loadShader(gl, type, source) {
						const shader = gl.createShader(type);
						gl.shaderSource(shader, source);
						gl.compileShader(shader);

						if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
							alert('An error occurred compiling the shader: ' + gl.getShaderInfoLog(shader));
							gl.deleteShader(shader);
							return null;
						}

						return shader;
					}

					function initBuffer(gl) {
						const positionBuffer = gl.createBuffer();
						gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
						const positions = [
							-1.0, -1.0,
							 1.0, -1.0,
							-1.0,  1.0,
							 1.0,  1.0,
						];
						gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
						return positionBuffer;
					}

				    // Vertex shader source.
					const vsSource = `
						const vec2 scale = vec2(0.5, 0.5);
						attribute vec4 aVertexPosition;

						varying highp vec2 vTextureCoord;

						void main () {
							gl_Position = aVertexPosition;
							vTextureCoord = aVertexPosition.xy * scale + scale;
						}
					`;

					// Fragment shader source.
					const fsSource = `
						varying highp vec2 vTextureCoord;

						uniform sampler2D uSampler;

						void main() {
							gl_FragColor = texture2D(uSampler, vTextureCoord);
						}
					`;

					const shaderProgram = initShaderProgram(gl, vsSource, fsSource);

					const programInfo = {
						program: shaderProgram,
						attribLocations: {
							vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
						},
						uniformLocations: {
							uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
						},
					};

					const buffer = initBuffer(gl);

					// The Module.webxr_blit_texture() function.
					return function (texture) {
						gl.useProgram(programInfo.program);

						gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
						gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
						gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

						gl.activeTexture(gl.TEXTURE0);
						gl.bindTexture(gl.TEXTURE_2D, texture);
						gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

						gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

						gl.bindTexture(gl.TEXTURE_2D, null);

					};
				})(Module.ctx);
			}

			const session_mode = UTF8ToString($0);
			const required_features = UTF8ToString($1).split(",").map((s) => { return s.trim() }).filter((s) => { return s !== "" });
			const optional_features = UTF8ToString($2).split(",").map((s) => { return s.trim() }).filter((s) => { return s !== "" });
			const requested_reference_space_types = UTF8ToString($3).split(",").map((s) => { return s.trim() });

			const session_init = {};
			if (required_features.length > 0) {
				session_init['requiredFeatures'] = required_features;
			}
			if (optional_features.length > 0) {
				session_init['optionalFeatures'] = optional_features;
			}

			navigator.xr.requestSession(session_mode, session_init).then(function (session) {
				Module['webxr_session'] = session;

				session.addEventListener('end', function (evt) {
					ccall('_emwebxr_on_session_ended', 'void', [], []);
				});

				const gl = Module['ctx'];
				gl.makeXRCompatible().then(function () {
					session.updateRenderState({
						baseLayer: new XRWebGLLayer(session, gl)
					});

					function onReferenceSpaceSuccess(reference_space, reference_space_type) {
						Module['webxr_space'] = reference_space;

						// Now that both Module.webxr_session and Module.webxr_space are set,
						// our monkey-patched requestAnimationFrame() should kick in.
						// When using the WebXR API Emulator, this gets picked up automatically,
						// however, in the Oculus Browser on the Quest, we need to pause and
						// resume the main loop.
						Module.Library_Browser_mainLoop.pause();
						window.setTimeout(function () { Module.Library_Browser_mainLoop.resume(); });

						ccall('_emwebxr_on_session_started', 'void', ['string'], [reference_space_type]);
					}
					
					function onReferenceSpaceFailure() {
						if (requested_reference_space_types.length === 0) {
							ccall('_emwebxr_on_session_failed', 'void', ['string'], ['Unable to get any of the requested reference space types']);
						}
						else {
							requestReferenceSpace();
						}
					}

					function requestReferenceSpace() {
						let reference_space_type = requested_reference_space_types.shift();
						session.requestReferenceSpace(reference_space_type)
							.then((refSpace) => { onReferenceSpaceSuccess(refSpace, reference_space_type); })
							.catch(onReferenceSpaceFailure);
					}

					requestReferenceSpace();
				}).catch(function (error) {
					ccall('_emwebxr_on_session_failed', 'void', ['string'], ['Unable to make WebGL context compatible with WebXR: ' + error]);
				});
			}).catch(function (error) {
				ccall('_emwebxr_on_session_failed', 'void', ['string'], ['Unable to start session: ' + error]);
			});
		}, session_mode.utf8().get_data(), required_features.utf8().get_data(), optional_features.utf8().get_data(), requested_reference_space_types.utf8().get_data());
		/* clang-format on */
	};

	return true;
};

void WebXRInterface::uninitialize() {
	if (initialized) {
		ARVRServer *arvr_server = ARVRServer::get_singleton();
		if (arvr_server != NULL) {
			// no longer our primary interface
			arvr_server->clear_primary_interface_if(this);
		}

		/* clang-format off */
		EM_ASM({
			if (Module.webxr_session) {
				Module.webxr_session.end()
					// Prevent exception when session has already ended.
					.catch((e) => { });
			}

			// Clean-up the textures we allocated for each view.
			const gl = Module.ctx;
			for (let i = 0; i < Module.webxr_textures.length; i++) {
				const texture = Module.webxr_textures[i];
				if (texture !== null) {
					gl.deleteTexture(texture);
				}
				Module.webxr_textures[i] = null;
				Module.webxr_texture_ids[i] = null;
			}

			// Resetting these will switch back to window.requestAnimationFrame() for the main loop.
			Module.webxr_session = null;
			Module.webxr_space = null;
			Module.webxr_frame = null;
			Module.webxr_pose = null;

			// Pause and restart main loop to activate it on all platforms.
			Module.Library_Browser_mainLoop.pause();
			window.setTimeout(function () { Module.Library_Browser_mainLoop.resume(); });
		});
		/* clang-format on */

		reference_space_type = "";

		initialized = false;
	};
};

bool WebXRInterface::_have_vr_support() {
	/* clang-format off */
	return EM_ASM_INT({
		return !!navigator.xr;
	});
	/* clang-format on */
}

bool WebXRInterface::_have_frame() {
	/* clang-format off */
	return (bool) EM_ASM_INT({
		return !!Module['webxr_session'] && !!Module['webxr_pose'];
	});
	/* clang-format on */
}

Size2 WebXRInterface::get_render_targetsize() {
	Size2 target_size;

	if (!initialized || !_have_frame()) {
		// As a default, use half the window size.
		target_size = OS::get_singleton()->get_window_size();
		target_size.width /= 2.0;
		return target_size;
	}

	/* clang-format off */
	int32_t* js_size = (int32_t*) EM_ASM_INT({
		const glLayer = Module['webxr_session'].renderState.baseLayer;
		const view = Module['webxr_pose'].views[0];
		const viewport = glLayer.getViewport(view);

		//console.log("targetsize javascript viewport:");
		//console.log(viewport.width + " " + viewport.height);

		let buf = Module._malloc(2 * 4);
		setValue(buf + 0, viewport.width, 'i32');
		setValue(buf + 4, viewport.height, 'i32');
		return buf;
	});
	/* clang-format on */

	target_size.width = js_size[0];
	target_size.height = js_size[1];

	free(js_size);

	return target_size;
};

Transform WebXRInterface::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	Transform transform_for_eye;

	ARVRServer *arvr_server = ARVRServer::get_singleton();
	ERR_FAIL_NULL_V(arvr_server, transform_for_eye);

	// @todo In 3DOF where we only have rotation, we should take the position from the p_cam_transform, I think?

	if (!initialized || !_have_frame()) {
		transform_for_eye = p_cam_transform;
		return transform_for_eye;
	}

	/* clang-format off */
	float* js_matrix = (float*) EM_ASM_INT({
		const views = Module['webxr_pose'].views;
		let matrix;
		if ($0 === 0) {
			matrix = Module['webxr_pose'].transform.matrix;
		}
		else {
			matrix = views[$0 - 1].transform.matrix;
		}
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, p_eye);
	/* clang-format on */

	//printf("Transform for eye: %f %f %f %f | %f %f %f %f | %f %f %f %f | %f %f %f %f\n",
	//	js_matrix[0], js_matrix[1], js_matrix[2], js_matrix[3],
	//	js_matrix[4], js_matrix[5], js_matrix[6], js_matrix[7],
	//	js_matrix[8], js_matrix[9], js_matrix[10], js_matrix[11],
	//	js_matrix[12], js_matrix[13], js_matrix[14], js_matrix[15]);

	transform_for_eye.basis.elements[0].x = js_matrix[0];
	transform_for_eye.basis.elements[1].x = js_matrix[1];
	transform_for_eye.basis.elements[2].x = js_matrix[2];
	transform_for_eye.basis.elements[0].y = js_matrix[4];
	transform_for_eye.basis.elements[1].y = js_matrix[5];
	transform_for_eye.basis.elements[2].y = js_matrix[6];
	transform_for_eye.basis.elements[0].z = js_matrix[8];
	transform_for_eye.basis.elements[1].z = js_matrix[9];
	transform_for_eye.basis.elements[2].z = js_matrix[10];
	transform_for_eye.origin.x = js_matrix[12];
	transform_for_eye.origin.y = js_matrix[13];
	transform_for_eye.origin.z = js_matrix[14];

	free(js_matrix);

	return p_cam_transform * arvr_server->get_reference_frame() * transform_for_eye;
};

CameraMatrix WebXRInterface::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	CameraMatrix eye;

	if (!initialized || !_have_frame()) {
		return eye;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	/* clang-format off */
	float* js_matrix = (float*) EM_ASM_INT({
		const matrix = Module['webxr_pose'].views[$0].projectionMatrix;
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, view_index);
	/* clang-format on */

	int k = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			eye.matrix[i][j] = js_matrix[k++];
		}
	}

	free(js_matrix);

	// Copied from godot_oculus_mobile's ovr_mobile_session.cpp
	//eye.matrix[2][2] = -(p_z_far + p_z_near) / (p_z_far - p_z_near);
	//eye.matrix[2][3] = -(2.0f * p_z_far * p_z_near) / (p_z_far - p_z_near);

	return eye;
}

unsigned int WebXRInterface::get_external_texture_for_eye(ARVRInterface::Eyes p_eye) {
	if (!_have_frame()) {
		return 0;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	/* clang-format off */
	return EM_ASM_INT({
		if (Module.webxr_texture_ids[$0]) {
			return Module.webxr_texture_ids[$0];
		}

		const glLayer = Module['webxr_session'].renderState.baseLayer;
		const view = Module['webxr_pose'].views[$0];
		const viewport = glLayer.getViewport(view);
		const gl = Module['ctx'];

		const texture = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, texture);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, viewport.width, viewport.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);

		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
		gl.bindTexture(gl.TEXTURE_2D, null);

		const texture_id = Module.Library_GL.getNewId(Module.Library_GL.textures);
		Module.Library_GL.textures[texture_id] = texture;
		Module.webxr_textures[$0] = texture;
		Module.webxr_texture_ids[$0] = texture_id;
		return texture_id;
	}, view_index);
	/* clang-format on */
}

void WebXRInterface::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	if (!initialized || !_have_frame()) {
		return;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	/* clang-format off */
	EM_ASM({
		const glLayer = Module['webxr_session'].renderState.baseLayer;
		const view = Module['webxr_pose'].views[$0];
		const viewport = glLayer.getViewport(view);
		const gl = Module['ctx'];

		// Bind to WebXR's framebuffer.
		gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);
		gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

		Module.webxr_blit_texture(Module.webxr_textures[$0]);

	}, view_index);
	/* clang-format on */
};

void WebXRInterface::process() {
	if (initialized) {
		//set_position_from_sensors();
	};
};

void WebXRInterface::notification(int p_what) {
	// nothing to do here, I guess we could pauze our sensors...
}

WebXRInterface::WebXRInterface() {
	initialized = false;
	session_mode = "inline";
	requested_reference_space_types = "local";
};

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (is_initialized()) {
		uninitialize();
	};
};

#endif // JAVASCRIPT_ENABLED
