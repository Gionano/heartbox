import { useState, useEffect, useRef } from 'react';
import { MessageForm } from './components/MessageForm';
import { HeartBackground } from './components/HeartBackground';
import { toast, Toaster } from 'sonner@2.0.3';
import mqtt from 'mqtt';

export interface MqttConfig {
  server: string;
  port: string;
  topic: string;
  clientId: string;
}

export default function App() {
  const [config] = useState<MqttConfig>({
    server: 'broker.hivemq.com',
    port: '8884',
    topic: 'topik-unik-anda/heartbox/pesan',
    clientId: 'HeartBox-WebApp-' + Math.random().toString(16).substring(2, 8)
  });

  const clientRef = useRef<mqtt.MqttClient | null>(null);
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    // Connect to MQTT broker
    const brokerUrl = `wss://${config.server}:${config.port}/mqtt`;
    
    try {
      const client = mqtt.connect(brokerUrl, {
        clientId: config.clientId,
        clean: true,
        reconnectPeriod: 1000,
        connectTimeout: 30 * 1000,
      });

      client.on('connect', () => {
        console.log('Connected to MQTT broker');
        setIsConnected(true);
        toast.success('MQTT Terhubung ke Server!', {
          description: `Server: ${config.server} | Topic: ${config.topic}`,
          duration: 5000,
        });
      });

      client.on('error', (err) => {
        console.error('MQTT Connection error:', err);
        setIsConnected(false);
        toast.error('Gagal terhubung ke MQTT Server', {
          description: 'Periksa konfigurasi server Anda',
        });
      });

      client.on('offline', () => {
        console.log('MQTT client is offline');
        setIsConnected(false);
      });

      client.on('reconnect', () => {
        console.log('Reconnecting to MQTT broker...');
      });

      clientRef.current = client;

      return () => {
        if (client) {
          client.end();
        }
      };
    } catch (error) {
      console.error('Failed to connect to MQTT:', error);
      toast.error('Gagal menginisialisasi MQTT');
    }
  }, [config.server, config.port, config.clientId, config.topic]);

  const handleSendMessage = (messageText: string) => {
    if (!clientRef.current || !isConnected) {
      toast.error('Tidak terhubung ke MQTT Server');
      return;
    }

    // Send message to MQTT broker
    const promise = new Promise((resolve, reject) => {
      clientRef.current?.publish(
        config.topic,
        messageText,
        { qos: 1, retain: false },
        (error) => {
          if (error) {
            console.error('Publish error:', error);
            reject(error);
          } else {
            console.log('Message published:', messageText);
            resolve(messageText);
          }
        }
      );
    });

    toast.promise(promise, {
      loading: 'Mengirim pesan...',
      success: 'Pesan terkirim ke HeartBox! 💕',
      error: 'Gagal mengirim pesan',
    });
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-pink-100 via-purple-100 to-pink-200 relative overflow-hidden">
      <Toaster position="top-center" richColors />
      <HeartBackground />
      
      <div className="relative z-10 container mx-auto px-4 py-8 max-w-2xl">
        <header className="text-center mb-12">
          <h1 className="text-pink-600 mb-2">💝 HeartBox Messenger 💝</h1>
          <p className="text-pink-700">Kirim pesan cinta ke heartbox kamu</p>
        </header>

        <MessageForm onSendMessage={handleSendMessage} />
      </div>
    </div>
  );
}