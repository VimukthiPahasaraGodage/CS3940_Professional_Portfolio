package com.example.smarthomeautomationsystem;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import org.json.JSONObject;

import java.net.URI;
import java.net.URISyntaxException;

import dev.gustavoavila.websocketclient.WebSocketClient;

public class MainActivity extends AppCompatActivity {

    private WebSocketClient webSocketClient;
    private TextView status;
    private Button b1;
    private Button b2;
    private Button b3;
    private Button b4;
    private TextView l1;
    private String logs = "";

    private String connectedColor = "#00FF00";
    private String disconnectedColor = "#FF0000";

    private String b1_s = "false";
    private String b2_s = "false";

    private final char MANUAL_DAYTIME_OFF = '1';
    private final char MANUAL_DAYTIME_ON = '2';
    private final char MANUAL_OVERRIDE_OFF = '3';
    private final char MANUAL_OVERRIDE_ON = '4';

    private Context context;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);

        status = findViewById(R.id.status);
        b1 = findViewById(R.id.b1);
        b2 = findViewById(R.id.b2);
        b3 = findViewById(R.id.b3);
        b4 = findViewById(R.id.b4);
        l1 = findViewById(R.id.logs);

        context = this;

        createWebSocketClient();

        b1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (b1_s.equals("true")){
                    webSocketClient.send("1");
                }else if (b1_s.equals("false")) {
                    webSocketClient.send("2");
                }
            }
        });

        b2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (b2_s.equals("true")){
                    webSocketClient.send("3");
                }else if (b2_s.equals("false")) {
                    webSocketClient.send("4");
                }
            }
        });

        b3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                webSocketClient.send("5");
            }
        });

        b4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                logs = "";
                l1.setText(logs);
            }
        });
    }

    private void createWebSocketClient() {
        URI uri;
        try {
            // Connect to local host
            uri = new URI("ws://192.168.1.25:81/websocket");
        }
        catch (URISyntaxException e) {
            e.printStackTrace();
            return;
        }

        webSocketClient = new WebSocketClient(uri) {
            @Override
            public void onOpen() {
                Log.i("WebSocket", "Session is starting");
                status.setText("Connected");
                status.setBackgroundColor(Color.parseColor(connectedColor));
            }

            @Override
            public void onTextReceived(String s) {
                Log.i("WebSocket", "Message received");
                final String message = s;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try{
                            JSONObject obj = new JSONObject(message);
                            b1_s = obj.getString("B1_s");
                            b2_s = obj.getString("B2_s");
                            String log = obj.getString("Log");
                            if (b1_s.equals("true")){
                                b1.setText("Manual control in daytime: ON");
                            }else if (b1_s.equals("false")){
                                b1.setText("Manual control in daytime: OFF");
                            }

                            if (b2_s.equals("true")){
                                b2.setText("Manual control in daytime: ON");
                            }else if (b2_s.equals("false")){
                                b2.setText("Manual control in daytime: OFF");
                            }

                            logs += log + "\n";
                            l1.setText(logs);
                        } catch (Exception e){
                            e.printStackTrace();
                        }
                    }
                });
            }

            @Override
            public void onBinaryReceived(byte[] data) {
            }

            @Override
            public void onPingReceived(byte[] data) {
            }

            @Override
            public void onPongReceived(byte[] data) {
            }

            @Override
            public void onException(Exception e) {
                Toast.makeText(context, "Exception occurred, " + e.getMessage(), Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onCloseReceived(int reason, String description) {
                status.setText("Disconnected");
                status.setBackgroundColor(Color.parseColor(disconnectedColor));
            }
        };

        webSocketClient.setConnectTimeout(10000);
        webSocketClient.setReadTimeout(60000);
        webSocketClient.enableAutomaticReconnection(5000);
        webSocketClient.connect();
    }
}
