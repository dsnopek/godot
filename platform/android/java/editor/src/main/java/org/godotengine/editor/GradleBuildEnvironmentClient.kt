/**************************************************************************/
/*  TermuxResultService.kt                                                */
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

package org.godotengine.editor

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import android.os.Message
import android.os.Messenger
import android.os.RemoteException
import android.util.Log
import org.godotengine.godot.variant.Callable

internal class GradleBuildEnvironmentClient(private val context: Context) {

	companion object {
		private const val TAG = "GradleBuildEnvironment"
	}

	private var bound: Boolean = false
	private var messenger: Messenger? = null
	private var connection = object : ServiceConnection {
		override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
			messenger = Messenger(service)
			bound = true

			Log.i(TAG, "Service connected")
			for (callable in connectionCallbacks) {
				callable.call()
			}
			connectionCallbacks.clear()
			connecting = false
		}

		override fun onServiceDisconnected(name: ComponentName?) {
			messenger = null
			bound = false
			Log.i(TAG, "Service disconnected")
		}
	}

	private val connectionCallbacks = mutableListOf<Callable>()
	private var connecting = false

	fun connect(callable: Callable): Boolean {
		connectionCallbacks.add(callable)
		if (connecting) {
			return true;
		}
		connecting = true;

		val intent = Intent()
			.setComponent(ComponentName(
				"org.godotengine.godot_gradle_build_environment",
				"org.godotengine.godot_gradle_build_environment.BuildEnvironmentService"
			))

		val info = context.applicationContext.packageManager.resolveService(intent, 0)
		if (info == null) {
			Log.e(TAG, "Unable to resolve service")
			return false
		}

		return context.bindService(intent, connection, Context.BIND_AUTO_CREATE)
	}

	fun disconnect() {
		if (bound) {
			context.unbindService(connection)
			bound = false
		}
	}

	fun execute(path: String, arguments: Array<String>, workDir: String, resultCallback: Callable): Boolean {
		if (messenger == null) {
			return false
		}

		// @todo We'll want to stash some ID into `arg1` for the result callback
		val msg: Message = Message.obtain(null, 1, 0,0)

		val data = Bundle()
		data.putString("path", path)
		data.putString("workDir", workDir)
		data.putStringArrayList("args", ArrayList(arguments.toList()))
		msg.data = data

		try {
			messenger?.send(msg)
		} catch (e: RemoteException) {
			Log.e(TAG, "Unable to execute command: $path $arguments")
			e.printStackTrace()
			return false;
		}

		return true
	}

}
