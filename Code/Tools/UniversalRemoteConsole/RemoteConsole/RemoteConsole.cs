/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Socket connection with game. See CryEngine's RemoteConsole.cpp for information on the Server


using System.Collections.Generic;
using System.Net.Sockets;
using System.Threading;

namespace RemoteConsole
{
    static class Constants
    {
        public const int DEFAULT_PORT = 4600;
        public const int DEFAULT_BUFFER = 4096;
        public const int RECONNECTION_TIME = 3000;
    }

    public enum EMessageType
    {
        eMT_Message = 0,
        eMT_Warning,
        eMT_Error,
    }

    public class SLogMessage
    {
        public SLogMessage(EMessageType type, string message)
        {
            Type = type;
            Message = message;
        }
        public EMessageType Type;
        public string Message;
    }

    interface IRemoteConsoleClientListener
    {
        void OnLogMessage(SLogMessage message);
        void OnAutoCompleteDone(List<string> autoCompleteList);
        void OnConnected();
        void OnDisconnected();
    }

    interface IRemoteConsoleClient
    {
        void Start();
        void Stop();
        void SetServer(string ip);
        void ExecuteConsoleCommand(string command);
        void ExecuteGameplayCommand(string command);
        void SetListener(IRemoteConsoleClientListener listener);
        void PumpEvents();
    }

    class SRemoteConsoleClientListenerState
    {
        public IRemoteConsoleClientListener listener = null;
        public bool connected = false;
        public bool autoCompleteSent = false;

        public void SetListener(IRemoteConsoleClientListener listener)
        {
            this.listener = listener;
            Reset();
        }

        public void Reset()
        {
            autoCompleteSent = false;
        }
    }


    class RemoteConsole : IRemoteConsoleClient
    {
        private enum EConsoleEventType
        {
            eCET_Noop = 0,
            eCET_Req,
            eCET_LogMessage,
            eCET_LogWarning,
            eCET_LogError,
            eCET_ConsoleCommand,
            eCET_AutoCompleteList,
            eCET_AutoCompleteListDone,

            eCET_Strobo_GetThreads,
            eCET_Strobo_ThreadAdd,
            eCET_Strobo_ThreadDone,

            eCET_Strobo_GetResult,
            eCET_Strobo_ResultStart,
            eCET_Strobo_ResultDone,

            eCET_Strobo_StatStart,
            eCET_Strobo_StatAdd,
            eCET_Strobo_IPStart,
            eCET_Strobo_IPAdd,
            eCET_Strobo_SymStart,
            eCET_Strobo_SymAdd,
            eCET_Strobo_CallstackStart,
            eCET_Strobo_CallstackAdd,

            eCET_GameplayEvent,
        }

        private class SCommandEvent
        {
            public SCommandEvent(EConsoleEventType type, string command)
            {
                Type = type;
                Command = command;
            }
            public EConsoleEventType Type;
            public string Command;
        }

				public int Port { get; private set; }
        private Thread thrd = null;
        private System.Net.Sockets.TcpClient clientSocket = null;
        byte[] inStream = new byte[Constants.DEFAULT_BUFFER];

        private string server = "";
        private List<SCommandEvent> commands = new List<SCommandEvent>();
        private List<string> autoComplete = new List<string>();
        private List<SLogMessage> messages = new List<SLogMessage>();

        private SRemoteConsoleClientListenerState listener = new SRemoteConsoleClientListenerState();

        private object locker = new object();
        private object clientLocker = new object();

        private volatile bool running = false;
        private volatile bool stopping = false;
        private volatile bool isConnected = false;
        private volatile bool autoCompleteIsDone = false;
        private volatile bool resetConnection = false;
        private int ticks = 0;

        public RemoteConsole()
        {
          clientSocket = null;
					Port = Constants.DEFAULT_PORT;
        }

				public bool SetPort(string port_)
				{
					int port = -1;
					try
					{
						port = System.Convert.ToInt32(port_);
					}
					catch (System.Exception)
					{
					}

					if (port != -1 && (!isConnected || resetConnection))
					{
						Port = port;
						return true;
					}
					return false;
				}

        public void Start()
        {
            if (!running)
            {
                running = true;
                thrd = new Thread(threadFct);
                thrd.Start();
            }
        }

        public void Stop()
        {
            if (running)
            {
                running = false;
                stopping = true;
                lock (clientLocker)
                {
                    if (clientSocket != null)
                    {
                        clientSocket.Close();
                        clientSocket = new System.Net.Sockets.TcpClient();
                    }
                }
                while (stopping)
                    System.Threading.Thread.Sleep(10);
                thrd = null;
            }
        }

        public void SetServer(string ip)
        {
            if (listener.listener != null)
            {
                listener.listener.OnDisconnected();
                listener.connected = false;
            }
            lock (locker)
            {
                server = ip;
                ticks = 0;
                autoCompleteIsDone = false;
                listener.Reset();
                resetConnection = true;
            }
        }

        public void ExecuteConsoleCommand(string command)
        {
            lock (locker)
            {
                commands.Add(new SCommandEvent(EConsoleEventType.eCET_ConsoleCommand, command));
            }
        }

        public void ExecuteGameplayCommand(string command)
        {
            lock (locker)
            {
                commands.Add(new SCommandEvent(EConsoleEventType.eCET_GameplayEvent, command));
            }
        }

        public void SetListener(IRemoteConsoleClientListener listener)
        {
            lock (locker)
            {
                this.listener.SetListener(listener);
            }
        }

        public void PumpEvents()
        {
            List<SLogMessage> msgs = null;
            List<string> autoCmplt = null;
            bool sendConn = false;
            bool isConn = false;
            IRemoteConsoleClientListener l = null;
            lock (locker)
            {
                l = listener.listener;
                msgs = new List<SLogMessage>(messages);
                messages.Clear();
                if (autoCompleteIsDone && !listener.autoCompleteSent)
                {
                    autoCmplt = new List<string>(autoComplete);
                    autoComplete.Clear();
                    listener.autoCompleteSent = true;
                }
                if (isConnected != listener.connected && !resetConnection)
                {
                    listener.connected = isConnected;
                    sendConn = true;
                    isConn = isConnected;
                }
            }
            if (l != null)
            {
                if (sendConn)
                {
                    if (isConn)
                        l.OnConnected();
                    else
                        l.OnDisconnected();
                }
                if (msgs != null)
                {
                    for (int i = 0; i < msgs.Count; ++i)
                        l.OnLogMessage(msgs[i]);
                }
                if (autoCmplt != null)
                {
                    l.OnAutoCompleteDone(autoCmplt);
                }
            }
        }

        private void threadFct()
        {
            while (running)
            {
                if (!isConnected || resetConnection)
                {
                    clearCommands();
                    if (ticks++ % Constants.RECONNECTION_TIME == 0)
                    {
                        lock (clientLocker)
                        {
                            if (clientSocket != null && clientSocket.Connected)
                                clientSocket.Close();
                            clientSocket = new System.Net.Sockets.TcpClient();
                        }
                        try
                        {
														clientSocket.Connect(server, Port);
                            NetworkStream stream = clientSocket.GetStream();
                            stream.ReadTimeout = 3000;
                            stream.WriteTimeout = 3000;
                            ticks = 0;
                        }
#if DEBUG
                        catch (System.Exception ex)
                        {
                            addLogMessage(EMessageType.eMT_Message, ex.Message);
                        }
#else
											catch (System.Exception)	{	}
#endif
                    }

                    isConnected = clientSocket != null ? clientSocket.Connected : false;
                    if (!isConnected)
                        autoCompleteIsDone = false;
                    if (resetConnection)
                        resetConnection = false;
                }
                else
                {
                    isConnected = processClient();
                }
            }
            stopping = false;
        }

        private bool readData(ref EConsoleEventType id, ref string data)
        {
            try
            {
                NetworkStream stream = clientSocket.GetStream();
                int ret = 0;
                while (true)
                {
                    ret += stream.Read(inStream, ret, Constants.DEFAULT_BUFFER - ret);
                    if (inStream[ret - 1] == '\0')
                        break;
                }
                string returndata = System.Text.Encoding.ASCII.GetString(inStream);
                id = (EConsoleEventType)(returndata[0] - '0');
                int index = returndata.IndexOf('\0');
                data = returndata.Substring(1, index - 1);
            }
            catch (System.Exception)
            {
                return false;
            }
            return true;
        }

        bool sendData(EConsoleEventType id, string data = "")
        {
            char cid = (char)((char)id + '0');
            string msg = "";
            msg += cid;
            msg += data;
            msg += "\0";
            try
            {
                byte[] outStream = System.Text.Encoding.ASCII.GetBytes(msg);
                NetworkStream stream = clientSocket.GetStream();
                stream.Write(outStream, 0, outStream.Length);
                stream.Flush();
            }
            catch (System.Exception)
            {
                return false;
            }
            return true;
        }

        private void addLogMessage(EMessageType type, string message)
        {
            lock (locker)
            {
                messages.Add(new SLogMessage(type, message));
            }
        }

        private void addAutoCompleteItem(string item)
        {
            lock (locker)
            {
                autoComplete.Add(item);
            }
        }

        private bool getCommand(ref SCommandEvent command)
        {
            bool res = false;
            lock (locker)
            {
                if (commands.Count > 0)
                {
                    command = commands[0];
                    commands.RemoveAt(0);
                    res = true;
                }
            }
            return res;
        }

        private void autoCompleteDone()
        {
            lock (locker)
            {
                autoCompleteIsDone = true;
            }
        }

        private void clearCommands()
        {
            lock (locker)
            {
                commands.Clear();
            }
        }

        private bool processClient()
        {
            EConsoleEventType eventType = EConsoleEventType.eCET_Noop;
            string data = "";
            if (!readData(ref eventType, ref data))
                return false;

            switch (eventType)
            {
                case EConsoleEventType.eCET_LogMessage:
                    addLogMessage(EMessageType.eMT_Message, data);
                    return sendData(EConsoleEventType.eCET_Noop);
                case EConsoleEventType.eCET_LogWarning:
                    addLogMessage(EMessageType.eMT_Warning, data);
                    return sendData(EConsoleEventType.eCET_Noop);
                case EConsoleEventType.eCET_LogError:
                    addLogMessage(EMessageType.eMT_Error, data);
                    return sendData(EConsoleEventType.eCET_Noop);
                case EConsoleEventType.eCET_AutoCompleteList:
                    addAutoCompleteItem(data);
                    return sendData(EConsoleEventType.eCET_Noop);
                case EConsoleEventType.eCET_AutoCompleteListDone:
                    autoCompleteDone();
                    return sendData(EConsoleEventType.eCET_Noop);
                case EConsoleEventType.eCET_Req:
                    SCommandEvent command = null;
                    if (getCommand(ref command))
                        return sendData(command.Type, command.Command);
                    else
                        return sendData(EConsoleEventType.eCET_Noop);
                default:
                    return sendData(EConsoleEventType.eCET_Noop);
            }
        }
    }
}
