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
import android.os.Handler
import android.os.IBinder
import android.os.Message
import android.os.Messenger
import android.os.RemoteException
import android.util.Log
import org.godotengine.godot.variant.Callable
import kotlin.collections.set

private const val MSG_EXECUTE_COMMAND = 1
private const val MSG_COMMAND_RESULT = 2

internal class GradleBuildEnvironmentClient(private val context: Context) {

	companion object {
		private const val TAG = "GradleBuildEnvironment"
	}

	private var bound: Boolean = false
	private var outgoingMessenger: Messenger? = null
	private var connection = object : ServiceConnection {
		override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
			outgoingMessenger = Messenger(service)
			bound = true

			Log.i(TAG, "Service connected")
			for (callable in connectionCallbacks) {
				callable.call()
			}
			connectionCallbacks.clear()
			connecting = false
		}

		override fun onServiceDisconnected(name: ComponentName?) {
			outgoingMessenger = null
			bound = false
			Log.i(TAG, "Service disconnected")
		}
	}

	private inner class IncomingHandler: Handler() {
		override fun handleMessage(msg: Message) {
			when (msg.what) {
				MSG_COMMAND_RESULT -> {
					this@GradleBuildEnvironmentClient.receiveCommandResult(msg)
				}
				else -> super.handleMessage(msg)
			}
		}
	}
	private val incomingMessenger = Messenger(IncomingHandler())

	private val connectionCallbacks = mutableListOf<Callable>()
	private var connecting = false
	private var executionId = 1000
	private val executionMap = HashMap<Int, Callable>()

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

	private fun getNextExecutionId(resultCallback: Callable): Int {
		val id = executionId++
		executionMap[id] = resultCallback
		return id
	}

	fun execute(path: String, arguments: Array<String>, binds: Array<String>, workDir: String, resultCallback: Callable): Boolean {
		if (outgoingMessenger == null) {
			return false
		}

		val msg: Message = Message.obtain(null, MSG_EXECUTE_COMMAND, getNextExecutionId(resultCallback),0)
		msg.replyTo = incomingMessenger

		val data = Bundle()
		data.putString("path", path)
		data.putString("workDir", workDir)
		data.putStringArrayList("args", ArrayList(arguments.toList()))
		msg.data = data

		try {
			outgoingMessenger?.send(msg)
		} catch (e: RemoteException) {
			Log.e(TAG, "Unable to execute command: $path $arguments")
			e.printStackTrace()
			return false;
		}

		return true
	}

	private fun receiveCommandResult(msg: Message) {
		val data = msg.data

		val exitCode = data.getInt("exitCode")
		val stdout = data.getString("stdout", "")
		val stderr = data.getString("stderr", "")

		Log.i(TAG, "We received $exitCode and $stdout and $stderr")

		val resultCallback = executionMap.remove(msg.arg1)
		resultCallback?.call(exitCode, stdout, stderr)
	}

}
