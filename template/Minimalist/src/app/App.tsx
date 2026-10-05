import { useState, useEffect, useRef } from 'react';
import { Heart, Send, CheckCircle, Wifi, WifiOff } from 'lucide-react';
import MessageForm from './components/MessageForm';
import MessageHistory from './components/MessageHistory';
import { Toaster, toast } from 'sonner@2.0.3';
import mqtt from 'mqtt';

interface Message {
  id: string;
  content: string;
  timestamp: Date;
  status: 'sending' | 'sent';
}

interface MqttSettings {
  server: string;
  port: string;
  topic: string;
  clientId: string;
}

export default function App() {
  const [messages, setMessages] = useState<Message[]>([]);
  const [mqttConnected, setMqttConnected] = useState(false);
  const [isConnecting, setIsConnecting] = useState(true);
  const clientRef = useRef<mqtt.MqttClient | null>(null);
  
  // Konfigurasi MQTT - ubah langsung di sini
  const mqttConfig: MqttSettings = {
    server: 'broker.hivemq.com',
    port: '8884',
    topic: 'topik-unik-anda/heartbox/pesan', //sebelum directory /heartbox/ isi nama pelanggan
    clientId: 'HeartBox-WebApp-' + Math.random().toString(16).substr(2, 8)
  };

  // Koneksi ke MQTT broker saat pertama kali load
  useEffect(() => {
    // Gunakan WebSocket untuk koneksi browser
    const brokerUrl = `wss://${mqttConfig.server}:${mqttConfig.port}/mqtt`;
    
    console.log('Connecting to MQTT broker:', brokerUrl);
    
    const client = mqtt.connect(brokerUrl, {
      clientId: mqttConfig.clientId,
      clean: true,
      reconnectPeriod: 1000,
    });

    client.on('connect', () => {
      console.log('Connected to MQTT broker');
      setMqttConnected(true);
      setIsConnecting(false);
      toast.success('Terhubung ke MQTT Server', {
        description: 'Siap mengirim pesan ke HeartBox'
      });
    });

    client.on('error', (err) => {
      console.error('MQTT connection error:', err);
      setMqttConnected(false);
      setIsConnecting(false);
      toast.error('Gagal terhubung ke MQTT Server', {
        description: err.message
      });
    });

    client.on('reconnect', () => {
      console.log('Reconnecting to MQTT broker...');
      setIsConnecting(true);
    });

    client.on('disconnect', () => {
      console.log('Disconnected from MQTT broker');
      setMqttConnected(false);
    });

    clientRef.current = client;

    return () => {
      if (clientRef.current) {
        clientRef.current.end();
      }
    };
  }, []);

  const handleSendMessage = (content: string) => {
    const newMessage: Message = {
      id: Date.now().toString(),
      content,
      timestamp: new Date(),
      status: 'sending'
    };

    setMessages(prev => [newMessage, ...prev]);

    if (!clientRef.current || !mqttConnected) {
      toast.error('Tidak terhubung ke MQTT Server', {
        description: 'Silakan tunggu koneksi terhubung'
      });
      setMessages(prev => prev.filter(msg => msg.id !== newMessage.id));
      return;
    }
    
    // Tampilkan notifikasi sedang mengirim
    toast.loading('Mengirim pesan ke HeartBox...', {
      id: newMessage.id
    });
    
    // Kirim pesan ke MQTT broker
    clientRef.current.publish(
      mqttConfig.topic,
      content,
      { qos: 1 },
      (error) => {
        if (error) {
          console.error('MQTT publish error:', error);
          toast.error('Gagal mengirim pesan!', {
            id: newMessage.id,
            description: error.message
          });
          setMessages(prev => prev.filter(msg => msg.id !== newMessage.id));
        } else {
          console.log('Message published to:', mqttConfig.topic, 'Content:', content);
          setMessages(prev =>
            prev.map(msg =>
              msg.id === newMessage.id ? { ...msg, status: 'sent' } : msg
            )
          );
          
          // Tampilkan notifikasi sukses
          toast.success('Pesan berhasil terkirim!', {
            id: newMessage.id,
            description: 'Pesan Anda telah dikirim ke HeartBox',
            duration: 3000
          });
        }
      }
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-[#6667AB] via-[#7B7CC7] to-[#9394D1]">
      <Toaster position="top-center" richColors />
      <div className="container mx-auto px-4 py-12 max-w-2xl">
        {/* MQTT Connection Status */}
        <div className="mb-6">
          <div className={`inline-flex items-center gap-2 px-4 py-2 rounded-full ${
            isConnecting 
              ? 'bg-yellow-500/20 border border-yellow-500/50' 
              : mqttConnected 
                ? 'bg-green-500/20 border border-green-500/50' 
                : 'bg-red-500/20 border border-red-500/50'
          }`}>
            {isConnecting ? (
              <>
                <div className="w-2 h-2 bg-yellow-400 rounded-full animate-pulse" />
                <span className="text-white text-sm">Menyambungkan ke MQTT Server...</span>
              </>
            ) : mqttConnected ? (
              <>
                <Wifi className="w-4 h-4 text-green-400" />
                <span className="text-white text-sm">Terhubung ke MQTT Server</span>
              </>
            ) : (
              <>
                <WifiOff className="w-4 h-4 text-red-400" />
                <span className="text-white text-sm">Terputus dari MQTT Server</span>
              </>
            )}
          </div>
        </div>

        {/* Header */}
        <div className="text-center mb-12">
          <h1 className="text-white mb-2">HeartBox MQTT</h1>
          <p className="text-white/80">Kirim pesan cinta Anda ke heartbox</p>
        </div>

        {/* Message Form */}
        <MessageForm onSend={handleSendMessage} />
      </div>
    </div>
  );
}