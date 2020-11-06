/*
 * @constructor
 */
function XRSession() {

}

/*
 * @constructor
 */
function XRViewerPose() {

}

/*
 * @constructor
 */
function XRFrame() {}

/*
 * @type {XRSession}
 */
XRFrame.prototype.session;

/*
 * @return {XRViewerPose?}
 */
XRFrame.prototype.getViewerPose = function(referenceSpace) {}

/**
 * @typedef {function(number, XRFrame): undefined}
 */
var XRFrameRequestCallback;

/*
 * @constructor
 */
function XRViewport() {}

/*
 * @constructor
 */
function XRView() {}

/*
 * @constructor
 */
function XRWebGLLayer(session, ctx) {}

/*
 * @param {XRView} view
 * @return {XRViewport}
 */
XRWebGLLayer.prototype.getViewport = function(view) {};
