/**************************************************************************/
/*  library_godot_webxr.js                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

const GodotWebXR = {
	$GodotWebXR__deps: ['$MainLoop', '$GL', '$GodotRuntime', '$runtimeKeepalivePush', '$runtimeKeepalivePop'],
	$GodotWebXR: {
		gl: null,

		session: null,
		gl_binding: null,
		disable_webxr_layers: false,
		layer: null,
		space: null,
		frame: null,
		pose: null,
		view_count: 1,
		input_sources: new Array(16),
		touches: new Array(5),
		onsimpleevent: null,

		// Some browsers (ex. Chrome on desktop with SteamVR) allow creating a
		// projection layer, but then never deliver any frames. So, if we don't get
		// a frame within this amount of (visible) time, we fallback on XRWebGLLayer.
		LAYERS_FRAME_TIMEOUT_MS: 2000,
		layers_watchdog: null,
		received_frame: false,

		startLayersWatchdog: () => {
			GodotWebXR.stopLayersWatchdog();
			GodotWebXR.received_frame = false;

			let visible_time = 0;
			let last_time = performance.now();
			GodotWebXR.layers_watchdog = window.setInterval(function () {
				const now = performance.now();
				const session = GodotWebXR.session;
				if (!session || GodotWebXR.received_frame || GodotWebXR.disable_webxr_layers) {
					GodotWebXR.stopLayersWatchdog();
					return;
				}

				// No frames are expected while the session is hidden, so don't count that time.
				if (session.visibilityState !== 'hidden') {
					visible_time += now - last_time;
				}
				last_time = now;

				if (visible_time >= GodotWebXR.LAYERS_FRAME_TIMEOUT_MS) {
					GodotWebXR.stopLayersWatchdog();
					GodotWebXR.fallbackToWebGLLayer();
				}
			}, 100);
		},

		stopLayersWatchdog: () => {
			if (GodotWebXR.layers_watchdog !== null) {
				window.clearInterval(GodotWebXR.layers_watchdog);
				GodotWebXR.layers_watchdog = null;
			}
		},

		fallbackToWebGLLayer: () => {
			GodotRuntime.print(`WebXR: No frames received within ${GodotWebXR.LAYERS_FRAME_TIMEOUT_MS}ms using a WebXR Layers projection layer (disabling WebXR Layers)`);

			// Remove the projection layer, so that we can switch to using a base layer.
			try {
				GodotWebXR.session.updateRenderState({ layers: [] });
			} catch (error) {
				console.log('WebXR DEBUG: fallbackToWebGLLayer(): clearing layers failed:', error); // eslint-disable-line no-console
			}

			GodotWebXR.disable_webxr_layers = true;
			GodotWebXR.gl_binding = null;
			GodotWebXR.layer = null;

			// This will trigger the XRWebGLLayer to get created.
			try {
				if (!GodotWebXR.getLayer()) {
					GodotRuntime.error('WebXR: Unable to create fallback XRWebGLLayer.');
				}
			} catch (error) {
				GodotRuntime.error(`WebXR: Unable to create fallback XRWebGLLayer: ${error}`);
			}
		},

		// Monkey-patch the requestAnimationFrame() used by Emscripten for the main
		// loop, so that we can swap it out for XRSession.requestAnimationFrame()
		// when an XR session is started.
		orig_requestAnimationFrame: null,
		// DEBUG: Frame counter used to limit per-frame logging.
		debug_frame_count: 0,
		debugFrameLog: (...args) => {
			// Log the first 10 frames, then every 300th frame.
			if (GodotWebXR.debug_frame_count < 10 || GodotWebXR.debug_frame_count % 300 === 0) {
				console.log(`WebXR DEBUG [frame ${GodotWebXR.debug_frame_count}]:`, ...args); // eslint-disable-line no-console
			}
		},

		requestAnimationFrame: (callback) => {
			if (GodotWebXR.session && GodotWebXR.space) {
				GodotWebXR.debugFrameLog('requestAnimationFrame(): using XRSession.requestAnimationFrame()');
				const onFrame = function (time, frame) {
					GodotWebXR.debugFrameLog('onFrame(): start, time =', time);
					GodotWebXR.received_frame = true;
					GodotWebXR.frame = frame;
					GodotWebXR.pose = frame.getViewerPose(GodotWebXR.space);
					GodotWebXR.debugFrameLog('onFrame(): pose =', GodotWebXR.pose, 'views =', GodotWebXR.pose ? GodotWebXR.pose.views.length : 'n/a');
					callback(time);
					GodotWebXR.debugFrameLog('onFrame(): callback returned');
					GodotWebXR.frame = null;
					GodotWebXR.pose = null;
					GodotWebXR.debug_frame_count++;
				};
				GodotWebXR.session.requestAnimationFrame(onFrame);
			} else {
				GodotWebXR.debugFrameLog('requestAnimationFrame(): using original requestAnimationFrame() (session =', !!GodotWebXR.session, ', space =', !!GodotWebXR.space, ')');
				GodotWebXR.orig_requestAnimationFrame(callback);
			}
		},
		monkeyPatchRequestAnimationFrame: (enable) => {
			console.log('WebXR DEBUG: monkeyPatchRequestAnimationFrame(', enable, ')'); // eslint-disable-line no-console
			if (GodotWebXR.orig_requestAnimationFrame === null) {
				GodotWebXR.orig_requestAnimationFrame = MainLoop.requestAnimationFrame;
			}
			MainLoop.requestAnimationFrame = enable
				? GodotWebXR.requestAnimationFrame
				: GodotWebXR.orig_requestAnimationFrame;
		},
		pauseResumeMainLoop: () => {
			// Once both GodotWebXR.session and GodotWebXR.space are set or
			// unset, our monkey-patched requestAnimationFrame() should be
			// enabled or disabled. When using the WebXR API Emulator, this
			// gets picked up automatically, however, in the Oculus Browser
			// on the Quest, we need to pause and resume the main loop.
			console.log('WebXR DEBUG: pauseResumeMainLoop(): pausing main loop'); // eslint-disable-line no-console
			MainLoop.pause();
			runtimeKeepalivePush();
			window.setTimeout(function () {
				console.log('WebXR DEBUG: pauseResumeMainLoop(): resuming main loop'); // eslint-disable-line no-console
				runtimeKeepalivePop();
				MainLoop.resume();
				console.log('WebXR DEBUG: pauseResumeMainLoop(): main loop resumed'); // eslint-disable-line no-console
			}, 0);
		},

		getLayer: () => {
			const new_view_count = (GodotWebXR.pose) ? GodotWebXR.pose.views.length : 1;
			let layer = GodotWebXR.layer;

			if (layer && (GodotWebXR.disable_webxr_layers || GodotWebXR.view_count === new_view_count)) {
				return layer;
			}

			console.log('WebXR DEBUG: getLayer(): need new layer, old view_count =', GodotWebXR.view_count, ', new_view_count =', new_view_count, ', had layer =', !!layer, ', disable_webxr_layers =', GodotWebXR.disable_webxr_layers); // eslint-disable-line no-console

			if (!GodotWebXR.session) {
				console.log('WebXR DEBUG: getLayer(): no session, returning null'); // eslint-disable-line no-console
				return null;
			}

			const gl = GodotWebXR.gl;

			if (GodotWebXR.disable_webxr_layers) {
				console.log('WebXR DEBUG: getLayer(): creating XRWebGLLayer'); // eslint-disable-line no-console
				layer = new XRWebGLLayer(GodotWebXR.session, gl, {
					antialias: false,
				});
				console.log('WebXR DEBUG: getLayer(): XRWebGLLayer created, calling updateRenderState()', layer); // eslint-disable-line no-console
				GodotWebXR.session.updateRenderState({ baseLayer: layer });
				console.log('WebXR DEBUG: getLayer(): updateRenderState() done'); // eslint-disable-line no-console
			} else {
				if (!GodotWebXR.gl_binding || !GodotWebXR.gl_binding.createProjectionLayer) {
					console.log('WebXR DEBUG: getLayer(): no gl_binding or createProjectionLayer, returning null'); // eslint-disable-line no-console
					return null;
				}

				const projection_layer_init = {
					textureType: new_view_count > 1 ? 'texture-array' : 'texture',
					colorFormat: gl.RGBA8,
					depthFormat: gl.DEPTH_COMPONENT24,
				};
				console.log('WebXR DEBUG: getLayer(): calling createProjectionLayer()', projection_layer_init); // eslint-disable-line no-console
				layer = GodotWebXR.gl_binding.createProjectionLayer(projection_layer_init);
				console.log('WebXR DEBUG: getLayer(): createProjectionLayer() returned, calling updateRenderState()', layer); // eslint-disable-line no-console
				GodotWebXR.session.updateRenderState({ layers: [layer] });
				console.log('WebXR DEBUG: getLayer(): updateRenderState() done'); // eslint-disable-line no-console
			}

			GodotWebXR.layer = layer;
			GodotWebXR.view_count = new_view_count;
			return layer;
		},

		getSubImage: () => {
			if (!GodotWebXR.pose || GodotWebXR.disable_webxr_layers) {
				return null;
			}
			const layer = GodotWebXR.getLayer();
			if (layer === null) {
				GodotWebXR.debugFrameLog('getSubImage(): getLayer() returned null');
				return null;
			}

			// Because we always use "texture-array" for multiview and "texture"
			// when there is only 1 view, it should be safe to only grab the
			// subimage for the first view.
			GodotWebXR.debugFrameLog('getSubImage(): calling getViewSubImage()');
			const subimage = GodotWebXR.gl_binding.getViewSubImage(layer, GodotWebXR.pose.views[0]);
			GodotWebXR.debugFrameLog('getSubImage(): getViewSubImage() returned', subimage);
			return subimage;
		},

		getViewport: () => {
			if (!GodotWebXR.pose) {
				return null;
			}
			if (GodotWebXR.disable_webxr_layers) {
				const layer = GodotWebXR.getLayer();
				if (layer === null) {
					return null;
				}
				return layer.getViewport(GodotWebXR.pose.views[0]);
			}
			const subimage = GodotWebXR.getSubImage();
			if (subimage === null) {
				return null;
			}
			return subimage.viewport;
		},

		// Some custom WebGL code for blitting our eye textures to the
		// framebuffer we get from WebXR.
		shaderPrograms: {},
		programInfos: {},
		buffer: null,
		sampler: null,
		// Vertex shader source.
		vsSource: `#version 300 es
			const vec2 scale = vec2(0.5, -0.5);
			const vec2 offset = vec2(0.5, 0.5);
			in vec4 aVertexPosition;

			out highp vec2 vTextureCoord;

			void main () {
				gl_Position = aVertexPosition;
				vTextureCoord = aVertexPosition.xy * scale + offset;
			}
		`,
		// Fragment shader source.
		fsSource: `#version 300 es
			precision highp float;

			in highp vec2 vTextureCoord;

			uniform sampler2D uSampler;

			out vec4 fragColor;

			void main() {
				fragColor = texture(uSampler, vTextureCoord);
			}
		`,
		// Fragment shader source for texture arrays (one layer per view).
		fsSourceArray: `#version 300 es
			precision highp float;
			precision highp sampler2DArray;

			in highp vec2 vTextureCoord;

			uniform sampler2DArray uSampler;
			uniform float uLayer;

			out vec4 fragColor;

			void main() {
				fragColor = texture(uSampler, vec3(vTextureCoord, uLayer));
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
				GodotRuntime.error(`Unable to initialize the shader program: ${gl.getProgramInfoLog(shaderProgram)}`);
				return null;
			}

			return shaderProgram;
		},
		loadShader: (gl, type, source) => {
			const shader = gl.createShader(type);
			gl.shaderSource(shader, source);
			gl.compileShader(shader);

			if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
				GodotRuntime.error(`An error occurred compiling the shader: ${gl.getShaderInfoLog(shader)}`);
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
				-1.0, 1.0,
				1.0, 1.0,
			];
			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);
			return positionBuffer;
		},
		initSampler: (gl) => {
			const sampler = gl.createSampler();
			gl.samplerParameteri(sampler, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
			gl.samplerParameteri(sampler, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
			gl.samplerParameteri(sampler, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
			gl.samplerParameteri(sampler, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
			return sampler;
		},
		getProgramInfo: (gl, is_array) => {
			const key = is_array ? 'array' : '2d';
			if (!GodotWebXR.shaderPrograms[key]) {
				const shaderProgram = GodotWebXR.initShaderProgram(gl, GodotWebXR.vsSource, is_array ? GodotWebXR.fsSourceArray : GodotWebXR.fsSource);
				GodotWebXR.shaderPrograms[key] = shaderProgram;
				GodotWebXR.programInfos[key] = {
					program: shaderProgram,
					attribLocations: {
						vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
					},
					uniformLocations: {
						uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
						uLayer: gl.getUniformLocation(shaderProgram, 'uLayer'),
					},
				};
			}
			return GodotWebXR.programInfos[key];
		},
		blitTexture: (gl, texture, is_array, layer) => {
			const programInfo = GodotWebXR.getProgramInfo(gl, is_array);
			if (GodotWebXR.buffer === null) {
				GodotWebXR.buffer = GodotWebXR.initBuffer(gl);
			}
			if (GodotWebXR.sampler === null) {
				GodotWebXR.sampler = GodotWebXR.initSampler(gl);
			}
			const texture_target = is_array ? gl.TEXTURE_2D_ARRAY : gl.TEXTURE_2D;

			const orig_program = gl.getParameter(gl.CURRENT_PROGRAM);
			gl.useProgram(programInfo.program);

			const orig_vertex_array = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
			gl.bindVertexArray(null);
			gl.bindBuffer(gl.ARRAY_BUFFER, GodotWebXR.buffer);
			gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
			gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

			gl.activeTexture(gl.TEXTURE0);
			gl.bindTexture(texture_target, texture);
			gl.bindSampler(0, GodotWebXR.sampler);
			gl.uniform1i(programInfo.uniformLocations.uSampler, 0);
			if (is_array) {
				gl.uniform1f(programInfo.uniformLocations.uLayer, layer);
			}

			const blend_enabled = gl.getParameter(gl.BLEND);
			if (blend_enabled) {
				gl.disable(gl.BLEND);
			}
			const depth_test_enabled = gl.getParameter(gl.DEPTH_TEST);
			if (depth_test_enabled) {
				gl.disable(gl.DEPTH_TEST);
			}
			const cull_face_enabled = gl.getParameter(gl.CULL_FACE);
			if (cull_face_enabled) {
				gl.disable(gl.CULL_FACE);
			}
			const scissor_test_enabled = gl.getParameter(gl.SCISSOR_TEST);
			if (scissor_test_enabled) {
				gl.disable(gl.SCISSOR_TEST);
			}
			const stencil_test_enabled = gl.getParameter(gl.STENCIL_TEST);
			if (stencil_test_enabled) {
				gl.disable(gl.STENCIL_TEST);
			}

			gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

			// Restore state.
			gl.bindSampler(0, null);
			gl.bindTexture(texture_target, null);
			gl.disableVertexAttribArray(programInfo.attribLocations.vertexPosition);
			gl.bindBuffer(gl.ARRAY_BUFFER, null);
			gl.bindVertexArray(orig_vertex_array);
			gl.useProgram(orig_program);
			if (blend_enabled) {
				gl.enable(gl.BLEND);
			}
			if (depth_test_enabled) {
				gl.enable(gl.DEPTH_TEST);
			}
			if (cull_face_enabled) {
				gl.enable(gl.CULL_FACE);
			}
			if (scissor_test_enabled) {
				gl.enable(gl.SCISSOR_TEST);
			}
			if (stencil_test_enabled) {
				gl.enable(gl.STENCIL_TEST);
			}
		},

		getTextureId: (texture) => {
			if (texture.name !== undefined) {
				return texture.name;
			}

			const id = GL.getNewId(GL.textures);
			texture.name = id;
			GL.textures[id] = texture;

			return id;
		},

		addInputSource: (input_source) => {
			let name = -1;
			if (input_source.targetRayMode === 'tracked-pointer' && input_source.handedness === 'left') {
				name = 0;
			} else if (input_source.targetRayMode === 'tracked-pointer' && input_source.handedness === 'right') {
				name = 1;
			} else {
				for (let i = 2; i < 16; i++) {
					if (!GodotWebXR.input_sources[i]) {
						name = i;
						break;
					}
				}
			}
			if (name >= 0) {
				GodotWebXR.input_sources[name] = input_source;
				input_source.name = name;

				// Find a free touch index for screen sources.
				if (input_source.targetRayMode === 'screen') {
					let touch_index = -1;
					for (let i = 0; i < 5; i++) {
						if (!GodotWebXR.touches[i]) {
							touch_index = i;
							break;
						}
					}
					if (touch_index >= 0) {
						GodotWebXR.touches[touch_index] = input_source;
						input_source.touch_index = touch_index;
					}
				}
			}
			return name;
		},

		removeInputSource: (input_source) => {
			if (input_source.name !== undefined) {
				const name = input_source.name;
				if (name >= 0 && name < 16) {
					GodotWebXR.input_sources[name] = null;
				}

				if (input_source.touch_index !== undefined) {
					const touch_index = input_source.touch_index;
					if (touch_index >= 0 && touch_index < 5) {
						GodotWebXR.touches[touch_index] = null;
					}
				}
				return name;
			}
			return -1;
		},

		getInputSourceId: (input_source) => {
			if (input_source !== undefined) {
				return input_source.name;
			}
			return -1;
		},

		getTouchIndex: (input_source) => {
			if (input_source.touch_index !== undefined) {
				return input_source.touch_index;
			}
			return -1;
		},
	},

	godot_webxr_is_supported__proxy: 'sync',
	godot_webxr_is_supported__sig: 'i',
	godot_webxr_is_supported: function () {
		return !!navigator.xr;
	},

	godot_webxr_is_session_supported__proxy: 'sync',
	godot_webxr_is_session_supported__sig: 'vii',
	godot_webxr_is_session_supported: function (p_session_mode, p_callback) {
		const session_mode = GodotRuntime.parseString(p_session_mode);
		const cb = GodotRuntime.get_func(p_callback);
		if (navigator.xr) {
			navigator.xr.isSessionSupported(session_mode).then(function (supported) {
				const c_str = GodotRuntime.allocString(session_mode);
				cb(c_str, supported ? 1 : 0);
				GodotRuntime.free(c_str);
			});
		} else {
			const c_str = GodotRuntime.allocString(session_mode);
			cb(c_str, 0);
			GodotRuntime.free(c_str);
		}
	},

	godot_webxr_initialize__deps: ['emscripten_webgl_get_current_context'],
	godot_webxr_initialize__proxy: 'sync',
	godot_webxr_initialize__sig: 'viiiiiiiiii',
	godot_webxr_initialize: function (p_session_mode, p_required_features, p_optional_features, p_requested_reference_spaces, p_disable_webxr_layers, p_on_session_started, p_on_session_ended, p_on_session_failed, p_on_input_event, p_on_simple_event) {
		console.log('WebXR DEBUG: godot_webxr_initialize(): start'); // eslint-disable-line no-console
		GodotWebXR.debug_frame_count = 0;
		GodotWebXR.monkeyPatchRequestAnimationFrame(true);

		const session_mode = GodotRuntime.parseString(p_session_mode);
		const required_features = GodotRuntime.parseString(p_required_features).split(',').map((s) => s.trim()).filter((s) => s !== '');
		const optional_features = GodotRuntime.parseString(p_optional_features).split(',').map((s) => s.trim()).filter((s) => s !== '');
		const requested_reference_space_types = GodotRuntime.parseString(p_requested_reference_spaces).split(',').map((s) => s.trim());
		const onstarted = GodotRuntime.get_func(p_on_session_started);
		const onended = GodotRuntime.get_func(p_on_session_ended);
		const onfailed = GodotRuntime.get_func(p_on_session_failed);
		const oninputevent = GodotRuntime.get_func(p_on_input_event);
		const onsimpleevent = GodotRuntime.get_func(p_on_simple_event);

		let disable_webxr_layers = !!p_disable_webxr_layers || typeof XRWebGLBinding === 'undefined';

		const session_init = {};
		if (required_features.length > 0) {
			session_init['requiredFeatures'] = required_features;
		}
		if (optional_features.length > 0) {
			session_init['optionalFeatures'] = optional_features;
		}

		console.log('WebXR DEBUG: godot_webxr_initialize(): session_mode =', session_mode, ', session_init =', JSON.stringify(session_init), ', requested_reference_space_types =', requested_reference_space_types, ', p_disable_webxr_layers =', p_disable_webxr_layers, ', XRWebGLBinding defined =', typeof XRWebGLBinding !== 'undefined', ', disable_webxr_layers =', disable_webxr_layers); // eslint-disable-line no-console
		console.log('WebXR DEBUG: godot_webxr_initialize(): calling navigator.xr.requestSession()'); // eslint-disable-line no-console

		navigator.xr.requestSession(session_mode, session_init).then(function (session) {
			console.log('WebXR DEBUG: requestSession() resolved', session, ', enabledFeatures =', 'enabledFeatures' in session ? Array.from(session.enabledFeatures) : 'n/a'); // eslint-disable-line no-console
			GodotWebXR.session = session;

			session.addEventListener('end', function (evt) {
				console.log('WebXR DEBUG: session "end" event'); // eslint-disable-line no-console
				GodotWebXR.stopLayersWatchdog();
				onended();
			});

			session.addEventListener('inputsourceschange', function (evt) {
				console.log('WebXR DEBUG: session "inputsourceschange" event, added =', evt.added.length, ', removed =', evt.removed.length); // eslint-disable-line no-console
				evt.added.forEach(GodotWebXR.addInputSource);
				evt.removed.forEach(GodotWebXR.removeInputSource);
			});

			['selectstart', 'selectend', 'squeezestart', 'squeezeend'].forEach((input_event, index) => {
				session.addEventListener(input_event, function (evt) {
					// Since this happens in-between normal frames, we need to
					// grab the frame from the event in order to get poses for
					// the input sources.
					GodotWebXR.frame = evt.frame;
					oninputevent(index, GodotWebXR.getInputSourceId(evt.inputSource));
					GodotWebXR.frame = null;
				});
			});

			session.addEventListener('visibilitychange', function (evt) {
				console.log('WebXR DEBUG: session "visibilitychange" event, visibilityState =', session.visibilityState); // eslint-disable-line no-console
				const c_str = GodotRuntime.allocString('visibility_state_changed');
				onsimpleevent(c_str);
				GodotRuntime.free(c_str);
			});

			// Store onsimpleevent so we can use it later.
			GodotWebXR.onsimpleevent = onsimpleevent;

			console.log('WebXR DEBUG: getting WebGL context'); // eslint-disable-line no-console
			const gl_context_handle = _emscripten_webgl_get_current_context();
			console.log('WebXR DEBUG: gl_context_handle =', gl_context_handle); // eslint-disable-line no-console
			const gl = GL.getContext(gl_context_handle).GLctx;
			GodotWebXR.gl = gl;
			console.log('WebXR DEBUG: got WebGL context', gl, ', calling gl.makeXRCompatible()'); // eslint-disable-line no-console

			gl.makeXRCompatible().then(function () {
				console.log('WebXR DEBUG: gl.makeXRCompatible() resolved'); // eslint-disable-line no-console
				if (!disable_webxr_layers) {
					try {
						console.log('WebXR DEBUG: creating XRWebGLBinding'); // eslint-disable-line no-console
						GodotWebXR.gl_binding = new XRWebGLBinding(session, gl);
						console.log('WebXR DEBUG: XRWebGLBinding created', GodotWebXR.gl_binding); // eslint-disable-line no-console
					} catch (error) {
						console.log('WebXR: Unable to create XRWebGLBinding (disabling WebXR Layers): ', error); // eslint-disable-line no-console
						disable_webxr_layers = true;
					}
				}

				if (!disable_webxr_layers && !GodotWebXR.gl_binding.createProjectionLayer) {
					console.log('WebXR: XRWebGLBinding.createProjectionLayer is missing (disabling WebXR Layers)'); // eslint-disable-line no-console
					disable_webxr_layers = true;
					GodotWebXR.gl_binding = null;
				}

				GodotWebXR.disable_webxr_layers = disable_webxr_layers;
				console.log('WebXR DEBUG: final disable_webxr_layers =', disable_webxr_layers, ', calling getLayer()'); // eslint-disable-line no-console

				// This will trigger the layer to get created.
				const layer = GodotWebXR.getLayer();
				console.log('WebXR DEBUG: getLayer() returned', layer); // eslint-disable-line no-console
				if (!layer) {
					throw new Error('Unable to create WebXR Layer.');
				}

				function onReferenceSpaceSuccess(reference_space, reference_space_type) {
					console.log('WebXR DEBUG: onReferenceSpaceSuccess(): reference_space_type =', reference_space_type, reference_space); // eslint-disable-line no-console
					GodotWebXR.space = reference_space;

					// Using reference_space.addEventListener() crashes when
					// using the polyfill with the WebXR Emulator extension,
					// so we set the event property instead.
					reference_space.onreset = function (evt) {
						const c_str = GodotRuntime.allocString('reference_space_reset');
						onsimpleevent(c_str);
						GodotRuntime.free(c_str);
					};

					// Now that both GodotWebXR.session and GodotWebXR.space are
					// set, we need to pause and resume the main loop for the XR
					// main loop to kick in.
					console.log('WebXR DEBUG: onReferenceSpaceSuccess(): calling pauseResumeMainLoop()'); // eslint-disable-line no-console
					GodotWebXR.pauseResumeMainLoop();

					// Make sure we actually get frames with the projection layer.
					if (!GodotWebXR.disable_webxr_layers) {
						console.log('WebXR DEBUG: starting layers watchdog'); // eslint-disable-line no-console
						GodotWebXR.startLayersWatchdog();
					}

					// Call in setTimeout() so that errors in the onstarted()
					// callback don't bubble up here and cause Godot to try the
					// next reference space.
					window.setTimeout(function () {
						console.log('WebXR DEBUG: calling onstarted() callback'); // eslint-disable-line no-console
						const reference_space_c_str = GodotRuntime.allocString(reference_space_type);
						const enabled_features = 'enabledFeatures' in session ? Array.from(session.enabledFeatures) : [];
						const enabled_features_c_str = GodotRuntime.allocString(enabled_features.join(','));
						const environment_blend_mode = 'environmentBlendMode' in session ? session.environmentBlendMode : '';
						const environment_blend_mode_c_str = GodotRuntime.allocString(environment_blend_mode);
						onstarted(reference_space_c_str, enabled_features_c_str, environment_blend_mode_c_str);
						GodotRuntime.free(reference_space_c_str);
						GodotRuntime.free(enabled_features_c_str);
						GodotRuntime.free(environment_blend_mode_c_str);
						console.log('WebXR DEBUG: onstarted() callback returned'); // eslint-disable-line no-console
					}, 0);
				}

				function requestReferenceSpace() {
					const reference_space_type = requested_reference_space_types.shift();
					console.log('WebXR DEBUG: requestReferenceSpace(', reference_space_type, ')'); // eslint-disable-line no-console
					session.requestReferenceSpace(reference_space_type)
						.then((refSpace) => {
							console.log('WebXR DEBUG: requestReferenceSpace(', reference_space_type, ') resolved'); // eslint-disable-line no-console
							onReferenceSpaceSuccess(refSpace, reference_space_type);
						})
						.catch((error) => {
							console.log('WebXR DEBUG: requestReferenceSpace(', reference_space_type, ') failed:', error); // eslint-disable-line no-console
							if (requested_reference_space_types.length === 0) {
								const c_str = GodotRuntime.allocString('Unable to get any of the requested reference space types');
								onfailed(c_str);
								GodotRuntime.free(c_str);
							} else {
								requestReferenceSpace();
							}
						});
				}

				requestReferenceSpace();
			}).catch(function (error) {
				console.log('WebXR DEBUG: error after gl.makeXRCompatible():', error); // eslint-disable-line no-console
				const c_str = GodotRuntime.allocString(`Unable to make WebGL context compatible with WebXR: ${error}`);
				onfailed(c_str);
				GodotRuntime.free(c_str);
			});
		}).catch(function (error) {
			console.log('WebXR DEBUG: requestSession() (or session setup) failed:', error); // eslint-disable-line no-console
			const c_str = GodotRuntime.allocString(`Unable to start session: ${error}`);
			onfailed(c_str);
			GodotRuntime.free(c_str);
		});
		console.log('WebXR DEBUG: godot_webxr_initialize(): end (async work pending)'); // eslint-disable-line no-console
	},

	godot_webxr_uninitialize__proxy: 'sync',
	godot_webxr_uninitialize__sig: 'v',
	godot_webxr_uninitialize: function () {
		console.log('WebXR DEBUG: godot_webxr_uninitialize()'); // eslint-disable-line no-console
		if (GodotWebXR.session) {
			GodotWebXR.session.end()
				// Prevent exception when session has already ended.
				.catch((e) => { });
		}

		GodotWebXR.stopLayersWatchdog();
		GodotWebXR.received_frame = false;
		GodotWebXR.disable_webxr_layers = false;

		GodotWebXR.session = null;
		GodotWebXR.gl_binding = null;
		GodotWebXR.layer = null;
		GodotWebXR.space = null;
		GodotWebXR.frame = null;
		GodotWebXR.pose = null;
		GodotWebXR.view_count = 1;
		GodotWebXR.input_sources = new Array(16);
		GodotWebXR.touches = new Array(5);
		GodotWebXR.onsimpleevent = null;

		// Disable the monkey-patched window.requestAnimationFrame() and
		// pause/restart the main loop to activate it on all platforms.
		GodotWebXR.monkeyPatchRequestAnimationFrame(false);
		GodotWebXR.pauseResumeMainLoop();
	},

	godot_webxr_get_view_count__proxy: 'sync',
	godot_webxr_get_view_count__sig: 'i',
	godot_webxr_get_view_count: function () {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return 1;
		}
		const view_count = GodotWebXR.pose.views.length;
		return view_count > 0 ? view_count : 1;
	},

	godot_webxr_get_render_target_size__proxy: 'sync',
	godot_webxr_get_render_target_size__sig: 'ii',
	godot_webxr_get_render_target_size: function (r_size) {
		const viewport = GodotWebXR.getViewport();
		GodotWebXR.debugFrameLog('godot_webxr_get_render_target_size(): viewport =', viewport);
		if (viewport === null) {
			return false;
		}

		GodotRuntime.setHeapValue(r_size + 0, viewport.width, 'i32');
		GodotRuntime.setHeapValue(r_size + 4, viewport.height, 'i32');

		return true;
	},

	godot_webxr_get_transform_for_view__proxy: 'sync',
	godot_webxr_get_transform_for_view__sig: 'iii',
	godot_webxr_get_transform_for_view: function (p_view, r_transform) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return false;
		}

		const views = GodotWebXR.pose.views;
		let matrix;
		if (p_view >= 0) {
			matrix = views[p_view].transform.matrix;
		} else {
			// For -1 (or any other negative value) return the HMD transform.
			matrix = GodotWebXR.pose.transform.matrix;
		}

		for (let i = 0; i < 16; i++) {
			GodotRuntime.setHeapValue(r_transform + (i * 4), matrix[i], 'float');
		}

		return true;
	},

	godot_webxr_get_projection_for_view__proxy: 'sync',
	godot_webxr_get_projection_for_view__sig: 'iii',
	godot_webxr_get_projection_for_view: function (p_view, r_transform) {
		if (!GodotWebXR.session || !GodotWebXR.pose) {
			return false;
		}

		const matrix = GodotWebXR.pose.views[p_view].projectionMatrix;
		for (let i = 0; i < 16; i++) {
			GodotRuntime.setHeapValue(r_transform + (i * 4), matrix[i], 'float');
		}

		return true;
	},

	godot_webxr_get_color_texture__proxy: 'sync',
	godot_webxr_get_color_texture__sig: 'i',
	godot_webxr_get_color_texture: function () {
		GodotWebXR.debugFrameLog('godot_webxr_get_color_texture(): start');
		const subimage = GodotWebXR.getSubImage();
		if (subimage === null) {
			GodotWebXR.debugFrameLog('godot_webxr_get_color_texture(): no subimage');
			return 0;
		}
		const color_texture_id = GodotWebXR.getTextureId(subimage.colorTexture);
		GodotWebXR.debugFrameLog('godot_webxr_get_color_texture(): texture id =', color_texture_id);
		return color_texture_id;
	},

	godot_webxr_get_depth_texture__proxy: 'sync',
	godot_webxr_get_depth_texture__sig: 'i',
	godot_webxr_get_depth_texture: function () {
		const subimage = GodotWebXR.getSubImage();
		if (subimage === null) {
			return 0;
		}
		if (!subimage.depthStencilTexture) {
			return 0;
		}
		return GodotWebXR.getTextureId(subimage.depthStencilTexture);
	},

	godot_webxr_get_velocity_texture__proxy: 'sync',
	godot_webxr_get_velocity_texture__sig: 'i',
	godot_webxr_get_velocity_texture: function () {
		const subimage = GodotWebXR.getSubImage();
		if (subimage === null) {
			return 0;
		}
		if (!subimage.motionVectorTexture) {
			return 0;
		}
		return GodotWebXR.getTextureId(subimage.motionVectorTexture);
	},

	godot_webxr_commit_render_target__proxy: 'sync',
	godot_webxr_commit_render_target__sig: 'vii',
	godot_webxr_commit_render_target: function (p_texture_id, p_layer_count) {
		if (!GodotWebXR.session || !GodotWebXR.pose || !GodotWebXR.disable_webxr_layers) {
			return;
		}

		GodotWebXR.debugFrameLog('godot_webxr_commit_render_target(): texture id =', p_texture_id, ', layer count =', p_layer_count);
		const glLayer = GodotWebXR.getLayer();
		const texture = GL.textures[p_texture_id];
		if (glLayer === null || !texture) {
			return;
		}

		const gl = GodotWebXR.gl;
		const is_array = p_layer_count > 1;

		const orig_framebuffer = gl.getParameter(gl.FRAMEBUFFER_BINDING);
		const orig_viewport = gl.getParameter(gl.VIEWPORT);

		// Bind to WebXR's framebuffer.
		gl.bindFramebuffer(gl.FRAMEBUFFER, glLayer.framebuffer);

		// Drawn rather than blitted: with foveated rendering (visionOS) the reported
		// viewports are in screen space and only rasterization goes through the rate map.
		GodotWebXR.pose.views.forEach((view, view_index) => {
			const viewport = glLayer.getViewport(view);
			gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);
			GodotWebXR.blitTexture(gl, texture, is_array, view_index);
		});

		// Restore state.
		gl.bindFramebuffer(gl.FRAMEBUFFER, orig_framebuffer);
		gl.viewport(orig_viewport[0], orig_viewport[1], orig_viewport[2], orig_viewport[3]);
	},

	godot_webxr_update_input_source__proxy: 'sync',
	godot_webxr_update_input_source__sig: 'iiiiiiiiiiiiiii',
	godot_webxr_update_input_source: function (p_input_source_id, r_target_pose, r_target_ray_mode, r_touch_index, r_has_grip_pose, r_grip_pose, r_has_standard_mapping, r_button_count, r_buttons, r_axes_count, r_axes, r_has_hand_data, r_hand_joints, r_hand_radii) {
		if (!GodotWebXR.session || !GodotWebXR.frame) {
			return 0;
		}

		if (p_input_source_id < 0 || p_input_source_id >= GodotWebXR.input_sources.length || !GodotWebXR.input_sources[p_input_source_id]) {
			return false;
		}

		const input_source = GodotWebXR.input_sources[p_input_source_id];
		const frame = GodotWebXR.frame;
		const space = GodotWebXR.space;

		// Target pose.
		const target_pose = frame.getPose(input_source.targetRaySpace, space);
		if (!target_pose) {
			// This can mean that the controller lost tracking.
			return false;
		}
		const target_pose_matrix = target_pose.transform.matrix;
		for (let i = 0; i < 16; i++) {
			GodotRuntime.setHeapValue(r_target_pose + (i * 4), target_pose_matrix[i], 'float');
		}

		// Target ray mode.
		let target_ray_mode = 0;
		switch (input_source.targetRayMode) {
		case 'gaze':
			target_ray_mode = 1;
			break;

		case 'tracked-pointer':
			target_ray_mode = 2;
			break;

		case 'screen':
			target_ray_mode = 3;
			break;

		default:
		}
		GodotRuntime.setHeapValue(r_target_ray_mode, target_ray_mode, 'i32');

		// Touch index.
		GodotRuntime.setHeapValue(r_touch_index, GodotWebXR.getTouchIndex(input_source), 'i32');

		// Grip pose.
		let has_grip_pose = false;
		if (input_source.gripSpace) {
			const grip_pose = frame.getPose(input_source.gripSpace, space);
			if (grip_pose) {
				const grip_pose_matrix = grip_pose.transform.matrix;
				for (let i = 0; i < 16; i++) {
					GodotRuntime.setHeapValue(r_grip_pose + (i * 4), grip_pose_matrix[i], 'float');
				}
				has_grip_pose = true;
			}
		}
		GodotRuntime.setHeapValue(r_has_grip_pose, has_grip_pose ? 1 : 0, 'i32');

		// Gamepad data (mapping, buttons and axes).
		let has_standard_mapping = false;
		let button_count = 0;
		let axes_count = 0;
		if (input_source.gamepad) {
			if (input_source.gamepad.mapping === 'xr-standard') {
				has_standard_mapping = true;
			}

			button_count = Math.min(input_source.gamepad.buttons.length, 10);
			for (let i = 0; i < button_count; i++) {
				GodotRuntime.setHeapValue(r_buttons + (i * 4), input_source.gamepad.buttons[i].value, 'float');
			}

			axes_count = Math.min(input_source.gamepad.axes.length, 10);
			for (let i = 0; i < axes_count; i++) {
				GodotRuntime.setHeapValue(r_axes + (i * 4), input_source.gamepad.axes[i], 'float');
			}
		}
		GodotRuntime.setHeapValue(r_has_standard_mapping, has_standard_mapping ? 1 : 0, 'i32');
		GodotRuntime.setHeapValue(r_button_count, button_count, 'i32');
		GodotRuntime.setHeapValue(r_axes_count, axes_count, 'i32');

		// Hand tracking data.
		let has_hand_data = false;
		if (input_source.hand && r_hand_joints !== 0 && r_hand_radii !== 0) {
			const hand_joint_array = new Float32Array(25 * 16);
			const hand_radii_array = new Float32Array(25);
			if (frame.fillPoses(input_source.hand.values(), space, hand_joint_array) && frame.fillJointRadii(input_source.hand.values(), hand_radii_array)) {
				GodotRuntime.heapCopy(HEAPF32, hand_joint_array, r_hand_joints);
				GodotRuntime.heapCopy(HEAPF32, hand_radii_array, r_hand_radii);
				has_hand_data = true;
			}
		}
		GodotRuntime.setHeapValue(r_has_hand_data, has_hand_data ? 1 : 0, 'i32');

		return true;
	},

	godot_webxr_get_visibility_state__proxy: 'sync',
	godot_webxr_get_visibility_state__sig: 'i',
	godot_webxr_get_visibility_state: function () {
		if (!GodotWebXR.session || !GodotWebXR.session.visibilityState) {
			return 0;
		}

		return GodotRuntime.allocString(GodotWebXR.session.visibilityState);
	},

	godot_webxr_get_bounds_geometry__proxy: 'sync',
	godot_webxr_get_bounds_geometry__sig: 'ii',
	godot_webxr_get_bounds_geometry: function (r_points) {
		if (!GodotWebXR.space || !GodotWebXR.space.boundsGeometry) {
			return 0;
		}

		const point_count = GodotWebXR.space.boundsGeometry.length;
		if (point_count === 0) {
			return 0;
		}

		const buf = GodotRuntime.malloc(point_count * 3 * 4);
		for (let i = 0; i < point_count; i++) {
			const point = GodotWebXR.space.boundsGeometry[i];
			GodotRuntime.setHeapValue(buf + ((i * 3) + 0) * 4, point.x, 'float');
			GodotRuntime.setHeapValue(buf + ((i * 3) + 1) * 4, point.y, 'float');
			GodotRuntime.setHeapValue(buf + ((i * 3) + 2) * 4, point.z, 'float');
		}
		GodotRuntime.setHeapValue(r_points, buf, 'i32');

		return point_count;
	},

	godot_webxr_get_frame_rate__proxy: 'sync',
	godot_webxr_get_frame_rate__sig: 'i',
	godot_webxr_get_frame_rate: function () {
		if (!GodotWebXR.session || GodotWebXR.session.frameRate === undefined) {
			return 0;
		}
		return GodotWebXR.session.frameRate;
	},

	godot_webxr_update_target_frame_rate__proxy: 'sync',
	godot_webxr_update_target_frame_rate__sig: 'vi',
	godot_webxr_update_target_frame_rate: function (p_frame_rate) {
		if (!GodotWebXR.session || GodotWebXR.session.updateTargetFrameRate === undefined) {
			return;
		}

		GodotWebXR.session.updateTargetFrameRate(p_frame_rate).then(() => {
			const c_str = GodotRuntime.allocString('display_refresh_rate_changed');
			GodotWebXR.onsimpleevent(c_str);
			GodotRuntime.free(c_str);
		});
	},

	godot_webxr_get_supported_frame_rates__proxy: 'sync',
	godot_webxr_get_supported_frame_rates__sig: 'ii',
	godot_webxr_get_supported_frame_rates: function (r_frame_rates) {
		if (!GodotWebXR.session || GodotWebXR.session.supportedFrameRates === undefined) {
			return 0;
		}

		const frame_rate_count = GodotWebXR.session.supportedFrameRates.length;
		if (frame_rate_count === 0) {
			return 0;
		}

		const buf = GodotRuntime.malloc(frame_rate_count * 4);
		for (let i = 0; i < frame_rate_count; i++) {
			GodotRuntime.setHeapValue(buf + (i * 4), GodotWebXR.session.supportedFrameRates[i], 'float');
		}
		GodotRuntime.setHeapValue(r_frame_rates, buf, 'i32');

		return frame_rate_count;
	},

};

autoAddDeps(GodotWebXR, '$GodotWebXR');
mergeInto(LibraryManager.library, GodotWebXR);
