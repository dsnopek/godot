/*************************************************************************/
/*  library_godot_audio.js                                               */
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
			if (this.session && this.space) {
				let onFrame = function (time, frame) {
					// @todo Do we actually need to do this?
					this.session = frame.session;

					this.frame = frame;
					this.pose = frame.getViewerPose(this.space);
					callback(time);
					this.frame = null;
					this.pose = null;
				};
				this.session.requestAnimationFrame(onFrame);
			}
			else {
				this.orig_requestAnimationFrame(callback);
			}
		},
		monkeyPatchRequestAnimationFrame: () => {
			if (this.orig_RequestAnimationFrame === null) {
				this.orig_requestAnimationFrame = Browser.requestAnimationFrame;
				Browser.requestAnimationFrame = this.requestAnimationFrame;
			}
		},
		pauseResumeMainLoop: () => {
			// Once both this.session and this.space are set or unset, our
			// monkey-patched requestAnimationFrame() should be enabled or
			// disabled. When using the WebXR API Emulator, this gets picked
			// up automatically, however, in the Oculus Browser on the Quest,
			// we need to pause and resume the main loop.
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
		blitTexture: (texture) => {
			const gl = Module.gl;

			if (this.shaderProgram === null) {
				this.shaderProgram = initShaderProgram(gl, vsSource, fsSource),
				this.programInfo = {
					program: this.shaderProgram,
					attribLocations: {
						vertexPosition: gl.getAttribLocation(this.shaderProgram, 'aVertexPosition'),
					},
					uniformLocations: {
						uSampler: gl.getUniformLocation(this.shaderProgram, 'uSampler'),
					},
				};
				this.buffer = initBuffer(gl);
			}

			gl.useProgram(programInfo.program);

			gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
			gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
			gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

			gl.activeTexture(gl.TEXTURE0);
			gl.bindTexture(gl.TEXTURE_2D, texture);
			gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

			gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

			gl.bindTexture(gl.TEXTURE_2D, null);
		},
	},

	godot_webxr_is_supported__proxy: 'sync',
	godot_webxr_is_supported__sig: 'i',
	godot_webxr_is_supported: function () {
		if (navigator.xr) {
			return 1;
		}
		return 0;
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

		const self = this;
		navigator.xr.requestSession(session_mode, session_init).then(function (session) {
			self.session = session;

			session.addEventListener('end', function (evt) {
				ccall('_emwebxr_on_session_ended', 'void', [], []);
			});

			const gl = Module.ctx;
			gl.makeXRCompatible().then(function () {
				session.updateRenderState({
					baseLayer: new XRWebGLLayer(session, gl)
				});

				function onReferenceSpaceSuccess(reference_space, reference_space_type) {
					self.space = reference_space;
					// Now that both self.session and self.space are set, we
					// need to pause and resume the main loop for the XR main
					// loop to kick in.
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

};

autoAddDeps(GodotWebXR, "$GodotWebXR");
mergeInto(LibraryManager.library, GodotWebXR);
