import { MessageCircle, Clock } from 'lucide-react';

interface Message {
  id: string;
  content: string;
  timestamp: Date;
  status: 'sending' | 'sent';
}

interface MessageHistoryProps {
  messages: Message[];
}

export default function MessageHistory({ messages }: MessageHistoryProps) {
  const formatTime = (date: Date) => {
    return date.toLocaleTimeString('id-ID', {
      hour: '2-digit',
      minute: '2-digit'
    });
  };

  return (
    <div className="mt-8">
      <h2 className="text-white mb-4 flex items-center gap-2">
        <MessageCircle className="w-5 h-5" />
        Riwayat Pesan
      </h2>
      <div className="space-y-3">
        {messages.map((msg) => (
          <div
            key={msg.id}
            className="bg-white/90 backdrop-blur-sm rounded-xl p-4 shadow-lg border border-white/50 transition-all hover:bg-white"
          >
            <div className="flex justify-between items-start gap-3">
              <p className="text-gray-800 flex-1">{msg.content}</p>
              <div className="flex items-center gap-1 text-xs text-gray-500 whitespace-nowrap">
                <Clock className="w-3 h-3" />
                {formatTime(msg.timestamp)}
              </div>
            </div>
            <div className="mt-2">
              <span
                className={`text-xs px-3 py-1 rounded-full ${
                  msg.status === 'sent'
                    ? 'bg-green-100 text-green-700'
                    : 'bg-yellow-100 text-yellow-700'
                }`}
              >
                {msg.status === 'sent' ? '✓ Terkirim' : '⟳ Mengirim...'}
              </span>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
