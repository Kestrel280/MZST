package com.example.androidserver;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private WifiManager wifiManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        Thread connectionListenerThread = new Thread(new Runnable() {
            @Override
            public void run() {
                new Server(wifiManager, Server.PORT);
            }
        });
        connectionListenerThread.start();

        while(true) {
            Log.i("mainThread", "Main Thread humming along...");
            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        /*try {
            Log.d("mainThread", "Blocking until connectionListenerThread finishes...");
            connectionListenerThread.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }*/
    }
}