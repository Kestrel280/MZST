package com.example.androidserver;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private static TextView debugTv;
    private WifiManager wifiManager;
    private Server server;

    public void debugBtn(View btnView) {
        ServerAction action = null;
        switch (btnView.getTag().toString()) {
            case "new":
                action = new ServerAction(ServerAction.ActionType.NEW);
                Log.i("main", "debug button 'new' clicked");
                break;
            case "load":
                action = new ServerAction(ServerAction.ActionType.LOAD).setCourseId(123);
                Log.i("main", "debug button 'load' clicked");
                break;
            case "save":
                action = new ServerAction(ServerAction.ActionType.SAVE);
                Log.i("main", "debug button 'save' clicked");
                break;
            case "reset":
                action = new ServerAction(ServerAction.ActionType.RESET);
                Log.i("main", "debug button 'reset' clicked");
                break;
        }
        if (action != null) {
            action.setRequesterId(0);
            server.sendAction(action);
        }
    }

    public static Handler uiMessageHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message inputMessage) {
            debugTv.setText(inputMessage.getData().getCharSequence("MESSAGE"));
        }
    };

    public static void debugMsgToAppView(String line) {
        Message msg = MainActivity.uiMessageHandler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putCharSequence("MESSAGE", line);
        msg.setData(bundle);
        MainActivity.uiMessageHandler.sendMessage(msg);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        debugTv = findViewById(R.id.debugText);
        debugTv.setText("Waiting for client");

        wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        server = new Server(wifiManager, Server.PORT);

        Log.i("main", "Main activity done");
    }
}