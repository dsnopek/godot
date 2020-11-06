/**
 * @type {XR}
 */
Navigator.prototype.xr;

/**
 * @constructor
 */
function XRSessionInit();

/**
 * @type {Array<string>}
 */
XRSessionInit.prototype.requiredFeatures;

/**
 * @type {Array<string>}
 */
XRSessionInit.prototype.optionalFeatures;

/**
 * @constructor
 */
function XR() {}

/**
 * @type {?function (Event)}
 */
XR.prototype.ondevicechanged;

/**
 * @param {string} mode 
 */
XR.prototype.isSessionSupported = function(mode) {}

/**
 * @param {string} mode 
 * @param {XRSessionInit} options 
 */
XR.prototype.requestSession = function(mode, options) {}

/**
 * @constructor
 */
function XRSession() {}

/**
 * @constructor
 */
function XRViewerPose() {}

/**
 * @constructor
 */
function XRFrame() {}

/**
 * @type {XRSession}
 */
XRFrame.prototype.session;

/**
 * @return {XRViewerPose?}
 */
XRFrame.prototype.getViewerPose = function(referenceSpace) {}

/**
 * @typedef {function(number, XRFrame): undefined}
 */
var XRFrameRequestCallback;

/**
 * @constructor
 */
function XRViewport() {}

/**
 * @constructor
 */
function XRView() {}

/**
 * @constructor
 */
function XRWebGLLayer(session, ctx) {}

/**
 * @param {XRView} view
 * @return {XRViewport?}
 */
XRWebGLLayer.prototype.getViewport = function(view) {};
