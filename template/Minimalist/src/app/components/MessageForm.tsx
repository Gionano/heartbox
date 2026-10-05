import { useState } from 'react';
import { Send } from 'lucide-react';

interface MessageFormProps {
  onSend: (message: string) => void;
}

export default function MessageForm({ onSend }: MessageFormProps) {
  const [message, setMessage] = useState('');

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (message.trim()) {
      onSend(message);
      setMessage('');
    }
  };

  return (
    <form onSubmit={handleSubmit} className="bg-white/95 backdrop-blur-sm rounded-2xl p-8 shadow-2xl border border-white/50">
      <label htmlFor="message" className="block text-[#6667AB] mb-3">
        Pesan Anda
      </label>
      <textarea
        id="message"
        value={message}
        onChange={(e) => setMessage(e.target.value)}
        placeholder="Tulis pesan untuk heartbox..."
        rows={4}
        className="w-full px-4 py-3 border-2 border-[#6667AB]/20 rounded-xl focus:outline-none focus:border-[#6667AB] focus:ring-2 focus:ring-[#6667AB]/20 resize-none transition-all"
      />
      <div className="flex justify-between items-center mt-4">
        <span className="text-sm text-gray-500">
          {message.length} karakter
        </span>
        <button
          type="submit"
          disabled={!message.trim()}
          className="bg-[#6667AB] text-white px-6 py-3 rounded-xl hover:bg-[#5556A0] disabled:opacity-50 disabled:cursor-not-allowed transition-all flex items-center gap-2 shadow-lg hover:shadow-xl"
        >
          <Send className="w-4 h-4" />
          Kirim ke HeartBox
        </button>
      </div>
    </form>
  );
}
