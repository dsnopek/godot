/*************************************************************************/
/*  library_godot_webxr.js                                               */
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
var GodotWebXR = {

	$GodotWebXR__deps: ['$Browser', '$GL'],
	$GodotWebXR: {
		texture_ids: [null, null],
		textures: [null, null],

		session: null,
		space: null,
		frame: null,
		pose: null,

		// Monkey-patch the requestAnimationFrame() used by Emscripten for the main
		// loop, so that we can swap it out for XRSession.requestAnimationFrame()
		// when an XR session is started.
		orig_RequestAnimationFrame: null,
		requestAnimationFrame: (callback) => {
			if (GodotWebXR.session && GodotWebXR.space) {
				let onFrame = function (time, frame) {
					// @todo Do we actually need to do this?
					GodotWebXR.session = frame.session;

					GodotWebXR.frame = frame;
					GodotWebXR.pose = frame.getViewerPose(GodotWebXR.space);
					callback(time);
					GodotWebXR.frame = null;
					GodotWebXR.pose = null;
				};
				GodotWebXR.session.requestAnimationFrame(onFrame);
			}
			else {
				GodotWebXR.orig_requestAnimationFrame(callback);
			}
		},
		monkeyPatchRequestAnimationFrame: () => {
			if (GodotWebXR.orig_RequestAnimationFrame === null) {
				GodotWebXR.orig_requestAnimationFrame = Browser.requestAnimationFrame;
				Browser.requestAnimationFrame = GodotWebXR.requestAnimationFrame;
			}
		},
		pauseResumeMainLoop: () => {
			// Once both GodotWebXR.session and GodotWebXR.space are set or
			// unset, our monkey-patched requestAnimationFrame() should be
			// enabled or disabled. When using the WebXR API Emulator, this
			// gets picked up automatically, however, in the Oculus Browser
			// on the Quest, we need to pause and resume the main loop.
			Browser.mainLoop.pause();
			window.setTimeout(function () { Browser.mainLoop.resume(); });
		},

		// Some custom WebGL code for blitting our eye textures to the
		// framebuffer we get from WebXR.
		shaderProgram: null,
		programInfo: null,
		buffer: null,
		// Vertex shader source.
		vsSource: `
			const vec2 scale = vec2(0.5, 0.5);
			attribute vec4 aVertexPosition;

			varying highp vec2 vTextureCoord;

			void main () {
				gl_Position = aVertexPosition;
				vTextureCoord = aVertexPosition.xy * scale + scale;
			}
		`,
		// Fragment shader source.
		fsSource: `
			varying highp vec2 vTextureCoord;

			uniform sampler2D uSampler;

			void main() {
				gl_FragColor = texture2D(uSampler, vTextureCoord);
			}
		`,

		initShaderProgram: (gl, vsSource, fsSource) => {
			const vertexShader = GodotWebXR.loadShader(gl, gl.VERTEX_SHADER, vsSource);
			const fragmentShader = GodotWebXR.loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

			const shaderProgram = gl.createProgram();
			gl.attachShader(shaderProgram, vertexShader);
			gl.attachShader(shaderProgram, fragmentShader);
			gl.linkProgram(shaderProgram);

			if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
				alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
				return null;
			}

			return shaderProgram;
		},
		loadShader: (gl, type, source) => {
			const shader = gl.createShader(type);
			gl.shaderSource(shader, source);
			gl.compileShader(shader);

			if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
				alert('An error occurred compiling the shader: ' + gl.getShaderInfoLog(shader));
				gl.deleteShader(shader);
				return null;
			}

			return shader;
		},
		initBuffer: (gl) => {
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
		},
		blitTexture: (gl, texture) => {
			if (GodotWebXR.shaderProgram === null) {
				GodotWebXR.shaderProgram = GodotWebXR.initShaderProgram(gl, GodotWebXR.vsSource, GodotWebXR.fsSource);
				GodotWebXR.programInfo = {
					program: GodotWebXR.shaderProgram,
					attribLocations: {
						vertexPosition: gl.getAttribLocation(GodotWebXR.shaderProgram, 'aVertexPosition'),
					},
					uniformLocations: {
						uSampler: gl.getUniformLocation(GodotWebXR.shaderProgram, 'uSampler'),
					},
				};
				GodotWebXR.buffer = GodotWebXR.initBuffer(gl);
			}

			const orig_program = gl.getParameter(gl.CURRENT_PROGRAM);
			gl.useProgram(GodotWebXR.shaderProgram);

			gl.bindBuffer(gl.ARRAY_BUFFER, GodotWebXR.buffer);
			gl.vertexAttribPointer(GodotWebXR.programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
			gl.enableVertexAttribArray(GodotWebXR.programInfo.attribLocations.vertexPosition);

			gl.activeTexture(gl.TEXTURE0);
			gl.bindTexture(gl.TEXTURE_2D, texture);
			gl.uniform1i(GodotWebXR.programInfo.uniformLocations.uSampler, 0);

			gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

			// Restore state.
			gl.bindTexture(gl.TEXTURE_2D, null);
			gl.disableVertexAttribArray(GodotWebXR.programInfo.attribLocations.vertexPosition);
			gl.bindBuffer(gl.ARRAY_BUFFER, null);
			gl.useProgram(orig_program);
		},

		// Holds the controllers list between function calls.
		controllers: [],

		// Gets an array with 0-2 items, where the left hand (or sole tracker)
		// is the first element, and the right hand is the second element.
		getControllers: () => {
			if (!GodotWebXR.session) {
				return [];
			}

			const controllers = [];
			for (let input_source of GodotWebXR.session.inputSources) {
				if (input_source.targetRayMode !== 'tracked-pointer') {
					continue;
				}
				if (input_source.handedness === 'right') {
					controllers[1] = input_source;
				}
				else if (input_source.handedness === 'left' || !controllers[0]) {
					controllers[0] = input_source;
				}
			}
			return controllers;
		},
	},

	godot_webxr_is_supported__proxy: 'sync',
	godot_webxr_is_supported__sig: 'i',
	godot_webxr_is_supported: function () {
		return !!navigator.xr;
	},

	godot_webxr_is_session_supported__proxy: 'sync',
	godot_webxr_is_session_supported__sig: 'vi',
	godot_webxr_is_session_supported: function (p_session_mode) {
		const session_mode = UTF8ToString(p_session_mode);
		if (navigator.xr) {
			navigator.xr.isSessionSupported(session_mode).then(function (supported) {
				ccall('_emwebxr_on_session_supported', 'void', ["string", "number"], [session_mode, supported]);
			});
		}
		else {
			ccall('_emwebxr_on_session_supported', 'void', ["string", "number"], [session_mode, false]);
		}
	},

	godot_webxr_initialize__proxy: 'sync',
	godot_webxr_initialize__sig: 'viiii',
	godot_webxr_initialize: function (p_session_mode, p_required_features, p_optional_features, p_requested_reference_spaces) {
		GodotWebXR.monkeyPatchRequestAnimationFrame();

		const session_mode = UTF8ToString(p_session_mode);
		const required_features = UTF8ToString(p_required_features).split(",").map((s) => { return s.trim() }).filter((s) => { return s !== "" });
		const optional_features = UTF8ToString(p_optional_features).split(",").map((s) => { return s.trim() }).filter((s) => { return s !== "" });
		const requested_reference_space_types = UTF8ToString(p_requested_reference_spaces).split(",").map((s) => { return s.trim() });

		const session_init = {};
		if (required_features.length > 0) {
			session_init['requiredFeatures'] = required_features;
		}
		if (optional_features.length > 0) {
			session_init['optionalFeatures'] = optional_features;
		}

		navigator.xr.requestSession(session_mode, session_init).then(function (session) {
			GodotWebXR.session = session;

			session.addEventListener('end', function (evt) {
				ccall('_emwebxr_on_session_ended', 'void', [], []);
			});

			session.addEventListener('inputsourceschange', function (evt) {
				let controller_changed = false;
				for (let lst of [evt.added, evt.removed]) {
					for (let input_source of lst) {
						if (input_source.targetRayMode === 'tracked-pointer') {
							controller_changed = true;
						}
					}
				}
				if (controller_changed) {
					ccall('_emwebxr_on_controller_changed', 'void', [], []);
				}
			});

			const gl = Module['ctx'];
			gl.makeXRCompatible().then(function () {
				session.updateRenderState({
					baseLayer: new XRWebGLLayer(session, gl)
				});

				function onReferenceSpaceSuccess(reference_space, reference_space_type) {
					GodotWebXR.space = reference_space;
					// Now that both GodotWebXR.session and GodotWebXR.space are
					// set, we need to pause and resume the main loop for the XR
					// main loop to kick in.
					GodotWebXR.pauseResumeMainLoop();
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
	},

	godot_webxr_uninitialize__proxy: 'sync',
	godot_webxr_uninitialize__sig: 'v',
	godot_webxr_uninitialize: function () {
		if (GodotWebXR.session) {
			GodotWebXR.session.end()
				// Prevent exception when session has already ended.
				.catch((e) => { });
		}

		// Clean-up the textures we allocated for each view.
		const gl = Module['ctx'];
		for (let i = 0; i < GodotWebXR.textures.length; i++) {
			const texture = GodotWebXR.textures[i];
			if (texture !== null) {
				gl.deleteTexture(texture);
			}
			GodotWebXR.textures[i] = null;
			GodotWebXR.texture_ids[i] = null;
		}

		// Resetting these will switch back to window.requestAnimationFrame() for the main loop.
		GodotWebXR.session = null;
		GodotWebXR.space = null;
		GodotWebXR.frame = null;
		GodotWebXR.pose = null;

		// Pause and restart main loop to activate it on all platforms.
		GodotWebXR.pauseResumeMainLoop();
	},

	godot_webxr_get_render_targetsize__proxy: 'sync',
	godot_webxr_get_render_targetsize__sig: 'i',
	godot_webxr_get_render_targetsize: function () {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return 0;
		}

		const glLayer = GodotWebXR.session.renderState.baseLayer;
		const view = GodotWebXR.pose.views[0];
		const viewport = glLayer.getViewport(view);

		let buf = Module['_malloc'](2 * 4);
		setValue(buf + 0, viewport.width, 'i32');
		setValue(buf + 4, viewport.height, 'i32');
		return buf;
	},

	godot_webxr_get_transform_for_eye__proxy: 'sync',
	godot_webxr_get_transform_for_eye__sig: 'ii',
	godot_webxr_get_transform_for_eye: function (p_eye) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return 0;
		}

		const views = GodotWebXR.pose.views;
		let matrix;
		if (p_eye === 0) {
			matrix = GodotWebXR.pose.transform.matrix;
		}
		else {
			matrix = views[p_eye - 1].transform.matrix;
		}
		let buf = Module['_malloc'](16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	},

	godot_webxr_get_projection_for_eye__proxy: 'sync',
	godot_webxr_get_projection_for_eye__sig: 'ii',
	godot_webxr_get_projection_for_eye: function (p_eye) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return 0;
		}

		const view_index = (p_eye == 2 /* ARVRInterface::EYE_RIGHT */) ? 1 : 0;
		const matrix = GodotWebXR.pose.views[view_index].projectionMatrix;
		let buf = Module['_malloc'](16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	},

	godot_webxr_get_external_texture_for_eye__proxy: 'sync',
	godot_webxr_get_external_texture_for_eye__sig: 'ii',
	godot_webxr_get_external_texture_for_eye: function (p_eye) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return 0;
		}

		const view_index = (p_eye == 2 /* ARVRInterface::EYE_RIGHT */) ? 1 : 0;
		if (GodotWebXR.texture_ids[view_index]) {
			return GodotWebXR.texture_ids[view_index];
		}

		const glLayer = GodotWebXR.session.renderState.baseLayer;
		const view = GodotWebXR.pose.views[view_index];
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

		const texture_id = GL.getNewId(GL.textures);
		GL.textures[texture_id] = texture;
		GodotWebXR.textures[view_index] = texture;
		GodotWebXR.texture_ids[view_index] = texture_id;
		return texture_id;
	},

	godot_webxr_commit_for_eye__proxy: 'sync',
	godot_webxr_commit_for_eye__sig: 'vi',
	godot_webxr_commit_for_eye: function (p_eye) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return;
		}

		const view_index = (p_eye == 2 /* ARVRInterface::EYE_RIGHT */) ? 1 : 0;
		const glLayer = GodotWebXR.session.renderState.baseLayer;
		const view = GodotWebXR.pose.views[view_index];
		const viewport = glLayer.getViewport(view);
		const gl = Module['ctx'];

		const orig_framebuffer = gl.getParameter(gl.FRAMEBUFFER_BINDING);
		const orig_viewport = gl.getParameter(gl.VIEWPORT);

		// Bind to WebXR's framebuffer.
		gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);
		gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

		GodotWebXR.blitTexture(gl, GodotWebXR.textures[view_index]);

		// Restore state.
		gl.bindFramebuffer(gl.FRAMEBUFFER, orig_framebuffer);
		gl.viewport(orig_viewport[0], orig_viewport[1], orig_viewport[2], orig_viewport[3]);
	},

	godot_webxr_sample_controller_data__proxy: 'sync',
	godot_webxr_sample_controller_data__sig: 'v',
	godot_webxr_sample_controller_data: function () {
		if (!GodotWebXR.session || !GodotWebXR.frame) {
			return;
		}
		GodotWebXR.controllers = GodotWebXR.getControllers();
	},

	godot_webxr_get_controller_count__proxy: 'sync',
	godot_webxr_get_controller_count__sig: 'i',
	godot_webxr_get_controller_count: function () {
		if (!GodotWebXR.session || !GodotWebXR.frame) {
			return 0;
		}
		return GodotWebXR.controllers.length;
	},

	godot_webxr_is_controller_connected__proxy: 'sync',
	godot_webxr_is_controller_connected__sig: 'ii',
	godot_webxr_is_controller_connected: function (p_controller) {
		if (!GodotWebXR.session || !GodotWebXR.frame) {
			return false;
		}
		return !!GodotWebXR.controllers[p_controller];
	},

	godot_webxr_get_controller_transform__proxy: 'sync',
	godot_webxr_get_controller_transform__sig: 'ii',
	godot_webxr_get_controller_transform: function (p_controller) {
		if (!GodotWebXR.session || !GodotWebXR.frame) {
			return 0;
		}

		const controller = GodotWebXR.controllers[p_controller];
		if (!controller) {
			return 0;
		}

		const frame = GodotWebXR.frame;
		const space = GodotWebXR.space;

		const pose = frame.getPose(controller.targetRaySpace, space);
		if (!pose) {
			// This can mean that the controller lost tracking.
			return 0;
		}
		const matrix = pose.transform.matrix;

		let buf = Module['_malloc'](16 * 4);
		for (let i = 0; i < 16; i++) {
			setValue(buf + (i * 4), matrix[i], 'float')
		}
		return buf;
	},

	godot_webxr_get_controller_buttons__proxy: 'sync',
	godot_webxr_get_controller_buttons__sig: 'ii',
	godot_webxr_get_controller_buttons: function (p_controller) {
		if (GodotWebXR.controllers.length === 0) {
			return 0;
		}

		const controller = GodotWebXR.controllers[p_controller];
		if (!controller || !controller.gamepad) {
			return 0;
		}

		const button_count = controller.gamepad.buttons.length;

		let buf = Module['_malloc']((button_count + 1) * 4);
		setValue(buf, button_count, 'i32');
		for (let i = 0; i < button_count; i++) {
			setValue(buf + 4 + (i * 4), controller.gamepad.buttons[i].value, 'float')
		}
		return buf;
	},

	godot_webxr_get_controller_axes__proxy: 'sync',
	godot_webxr_get_controller_axes__sig: 'ii',
	godot_webxr_get_controller_axes: function (p_controller) {
		if (GodotWebXR.controllers.length === 0) {
			return 0;
		}

		const controller = GodotWebXR.controllers[p_controller];
		if (!controller || !controller.gamepad) {
			return 0;
		}

		const axes_count = controller.gamepad.axes.length;

		let buf = Module['_malloc']((axes_count + 1) * 4);
		setValue(buf, axes_count, 'i32');
		for (let i = 0; i < axes_count; i++) {
			setValue(buf + 4 + (i * 4), controller.gamepad.axes[i], 'float')
		}
		return buf;
	},

};

autoAddDeps(GodotWebXR, "$GodotWebXR");
mergeInto(LibraryManager.library, GodotWebXR);
