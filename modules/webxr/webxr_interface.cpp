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

#include <stdlib.h>
#include "webxr_interface.h"
#include "core/os/input.h"
#include "core/os/os.h"
#include "servers/visual/visual_server_globals.h"
#include "emscripten.h"

StringName WebXRInterface::get_name() const {
	return "WebXR";
};

int WebXRInterface::get_capabilities() const {
	return ARVRInterface::ARVR_STEREO;
};

void WebXRInterface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("print_debug"), &WebXRInterface::print_debug);
}

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
		/* clang-format off */
		bool vr_supported = EM_ASM_INT({
			return !!navigator.xr;
		});
		/* clang-format on */

		if (!vr_supported) {
			return false;
		}

		// make this our primary interface
		arvr_server->set_primary_interface(this);

		initialized = true;

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
						gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
						gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
						gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

						gl.useProgram(programInfo.program);

						gl.activeTexture(gl.TEXTURE0);
						gl.bindTexture(gl.TEXTURE_2D, texture);
						gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

						gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

						gl.bindTexture(gl.TEXTURE_2D, null);

					};
				})(Module.ctx);
			}

			// @todo Calling session supported and getting the callback should be up to the developer using this interface
			navigator.xr.isSessionSupported('immersive-vr').then(function () {
				navigator.xr.requestSession('immersive-vr').then(function (session) {
					Module['webxr_session'] = session;
					const gl = Module['ctx'];
					gl.makeXRCompatible().then(function () {
						session.updateRenderState({
							baseLayer: new XRWebGLLayer(session, gl)
						});
						// @todo Reference space type needs to be configurable, and have automatic fallbacks.
						session.requestReferenceSpace('local').then(function (refSpace) {
							Module['webxr_space'] = refSpace;

							// Now that both Module.webxr_session and Module.webxr_space are set,
							// our monkey-patched requestAnimationFrame() should kick in.
							// When using the WebXR API Emulator, this gets picked up automatically,
							// however, in the Oculus Browser on the Quest, we need to pause and
							// resume the main loop.
							Module.Library_Browser_mainLoop.pause();
							window.setTimeout(function () { Module.Library_Browser_mainLoop.resume(); });
						});
					});
				});
			});
		});
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

		EM_ASM({
			Module.webxr_session.end();

			// Resetting these will switch back to window.requestAnimationFrame() for the main loop.
			// @todo Do we need to pause/resume the main loop here too?
			Module.webxr_session = null;
			Module.webxr_space = null;
			Module.webxr_frame = null;
			Module.webxr_pose = null;

			// @todo Clean-up the textures we allocated for each view
		});

		initialized = false;
	};
};

bool WebXRInterface::_have_frame() {
	return (bool) EM_ASM_INT({
		return !!Module['webxr_session'] && !!Module['webxr_pose'];
	});
}

Size2 WebXRInterface::get_render_targetsize() {
	Size2 target_size;

	if (!initialized || !_have_frame()) {
		// As a default, use half the window size.
		target_size = OS::get_singleton()->get_window_size();
		target_size.width /= 2.0;
		return target_size;
	}

	int32_t* js_size = (int32_t*) EM_ASM_INT({
		const glLayer = Module['webxr_frame'].session.renderState.baseLayer;
		const view = Module['webxr_pose'].views[0];
		const viewport = glLayer.getViewport(view);

		//console.log("targetsize javascript viewport:");
		//console.log(viewport.width + " " + viewport.height);

		let buf = Module._malloc(2 * 4);
		setValue(buf + 0, viewport.width, 'i32');
		setValue(buf + 4, viewport.height, 'i32');
		return buf;
	});

	target_size.width = js_size[0];
	target_size.height = js_size[1];

	free(js_size);

	return target_size;
};

Transform WebXRInterface::get_transform_for_eye(ARVRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	Transform transform_for_eye;

	// @todo In 3DOF where we only have rotation, we should take the position from the p_cam_transform, I think?

	if (!initialized || !_have_frame()) {
		transform_for_eye = p_cam_transform;
		return transform_for_eye;
	}

	float* js_matrix = (float*) EM_ASM_INT({
		const views = Module['webxr_pose'].views;
		let matrix;
		if ($0 === 0) {
			matrix = Module['webxr_pose'].transform.matrix;
		}
		else {
			matrix = views[$0 - 1].transform.inverse.matrix;
		}
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, p_eye);

	transform_for_eye.basis.elements[0].x = js_matrix[0];
	transform_for_eye.basis.elements[0].y = js_matrix[1];
	transform_for_eye.basis.elements[0].z = js_matrix[2];
	transform_for_eye.basis.elements[1].x = js_matrix[4];
	transform_for_eye.basis.elements[1].y = js_matrix[5];
	transform_for_eye.basis.elements[1].z = js_matrix[6];
	transform_for_eye.basis.elements[2].x = js_matrix[8];
	transform_for_eye.basis.elements[2].y = js_matrix[9];
	transform_for_eye.basis.elements[2].z = js_matrix[10];
	transform_for_eye.origin.x = js_matrix[12];
	transform_for_eye.origin.y = js_matrix[13];
	transform_for_eye.origin.z = js_matrix[14];

	if (p_eye != ARVRInterface::EYE_MONO) {
		transform_for_eye.origin.x *= -1.0;
		transform_for_eye.origin.y *= -1.0;
		transform_for_eye.origin.z *= -1.0;
	}

	free(js_matrix);

	ARVRServer *arvr_server = ARVRServer::get_singleton();

	return p_cam_transform * arvr_server->get_reference_frame() * transform_for_eye;
};

CameraMatrix WebXRInterface::get_projection_for_eye(ARVRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	CameraMatrix eye;

	if (!initialized || !_have_frame()) {
		return eye;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	float* js_matrix = (float*) EM_ASM_INT({
		const matrix = Module['webxr_pose'].views[$0].projectionMatrix;
		let buf = Module._malloc(16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	}, view_index);

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
	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	return EM_ASM_INT({
		if (Module.webxr_texture_ids[$0]) {
			return Module.webxr_texture_ids[$0];
		}

		const glLayer = Module['webxr_frame'].session.renderState.baseLayer;
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
}

void WebXRInterface::commit_for_eye(ARVRInterface::Eyes p_eye, RID p_render_target, const Rect2 &p_screen_rect) {
	if (!initialized || !_have_frame()) {
		return;
	}

	int view_index = (p_eye == ARVRInterface::EYE_RIGHT) ? 1 : 0;

	EM_ASM({
		const glLayer = Module['webxr_frame'].session.renderState.baseLayer;
		const view = Module['webxr_pose'].views[$0];
		const viewport = glLayer.getViewport(view);
		const gl = Module['ctx'];

		// Bind to WebXR's framebuffer.
		gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);
		gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

		Module.webxr_blit_texture(Module.webxr_textures[$0]);

	}, view_index);
};

void WebXRInterface::process() {
	if (initialized) {
		//set_position_from_sensors();
	};
};

void WebXRInterface::notification(int p_what){
	// nothing to do here, I guess we could pauze our sensors...
}

void WebXRInterface::print_debug() const {
	EM_ASM({
		console.log('-- WebXRInterface debug --');
		console.log('Headset transform');
		console.log(Module['webxr_pose'].transform);
		console.log('View 1 transform');
		console.log(Module['webxr_pose'].views[0].transform.inverse);
		console.log('View 2 transform');
		console.log(Module['webxr_pose'].views[1].transform.inverse);
	});
}

WebXRInterface::WebXRInterface() {
	initialized = false;
};

WebXRInterface::~WebXRInterface() {
	// and make sure we cleanup if we haven't already
	if (is_initialized()) {
		uninitialize();
	};
};

#endif // JAVASCRIPT_ENABLED

