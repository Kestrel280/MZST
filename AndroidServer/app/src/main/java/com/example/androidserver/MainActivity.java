package com.example.androidserver;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import java.util.HashMap;

public class MainActivity extends AppCompatActivity {

    private static HashMap<String, TextView> textViews;
    private WifiManager wifiManager;
    private Server server;
    private Server.ClientHandler debugSelectedHandler;

    public void debugBtn(View btnView) {
        ServerAction action = null;
        ServerMessage msg;
        EditText stringInputView = findViewById(R.id.debugStringInput);
        EditText numberInputView = findViewById(R.id.debugNumberInput);
        String stringInput = stringInputView.getText().toString();
        int numberInput;
        try {
            numberInput = Integer.parseInt(numberInputView.getText().toString());
        } catch (Exception e) {
            numberInput = 0;
        }
        switch (btnView.getTag().toString()) {
            case "new":
                action = new ServerAction(ServerAction.ActionType.NEW);
                break;
            case "load":
                action = new ServerAction(ServerAction.ActionType.LOAD).setCourseId(123);
                break;
            case "save":
                action = new ServerAction(ServerAction.ActionType.SAVE);
                break;
            case "reset":
                action = new ServerAction(ServerAction.ActionType.RESET);
                break;
            case "send":
                msg = new ServerMessage(stringInput);
                debugSelectedHandler.postMsg(numberInput, msg);
                break;
            case "broadcast":
                msg = new ServerMessage(stringInput);
                debugSelectedHandler.postBroadcast(msg);
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
            TextView view = textViews.get(inputMessage.getData().getCharSequence("VIEW"));
            view.setText(inputMessage.getData().getCharSequence("MESSAGE"));
        }
    };

    public static void debugMsgToAppView(String line, String viewName) {
        Message msg = MainActivity.uiMessageHandler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putCharSequence("MESSAGE", line);
        bundle.putCharSequence("VIEW", viewName);
        msg.setData(bundle);
        MainActivity.uiMessageHandler.sendMessage(msg);
    }

    private final RadioGroup.OnCheckedChangeListener radioListener = new RadioGroup.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(RadioGroup group, int checkedId) {
            RadioButton radioButton = group.findViewById(checkedId);

            switch (radioButton.getTag().toString()) {
                case "users": debugSelectedHandler = server.userHandler; break;
                case "nodes": debugSelectedHandler = server.nodeHandler; break;
                case "transmitters": debugSelectedHandler = server.transmitterHandler; break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textViews = new HashMap<>();
        textViews.put("debug", findViewById(R.id.debugText));
        textViews.put("state", findViewById(R.id.debugState));
        textViews.get("debug").setText("Waiting for clients");

        wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        server = new Server(wifiManager, Server.PORT);

        RadioGroup radioGrp = findViewById(R.id.debugRadioGrp);
        radioGrp.setOnCheckedChangeListener(radioListener);
        radioGrp.check(R.id.dbgRadioUsers);

        Log.i("main", "Main activity done");
    }

    @Override
    protected void onStop() {
        server.shutdown();
        super.onStop();
    }

    public void triggerRebirth(View view) {
        server.shutdown();

        Context context = getApplicationContext();
        PackageManager packageManager = context.getPackageManager();
        Intent intent = packageManager.getLaunchIntentForPackage(context.getPackageName());
        ComponentName componentName = intent.getComponent();
        Intent mainIntent = Intent.makeRestartActivityTask(componentName);
        // Required for API 34 and later
        // Ref: https://developer.android.com/about/versions/14/behavior-changes-14#safer-intents
        mainIntent.setPackage(context.getPackageName());
        context.startActivity(mainIntent);
        Runtime.getRuntime().exit(0);
    }
}