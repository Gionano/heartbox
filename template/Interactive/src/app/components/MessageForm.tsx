import { useState } from 'react';
import { motion, AnimatePresence } from 'motion/react';
import { Heart, Send, Sparkles } from 'lucide-react';

interface MessageFormProps {
  onSendMessage: (message: string) => void;
}

export function MessageForm({ onSendMessage }: MessageFormProps) {
  const [message, setMessage] = useState('');
  const [showHearts, setShowHearts] = useState(false);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (message.trim()) {
      onSendMessage(message);
      setMessage('');
      setShowHearts(true);
      setTimeout(() => setShowHearts(false), 2000);
    }
  };

  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.5, delay: 0.2 }}
      className="bg-white/80 backdrop-blur-sm rounded-3xl p-6 shadow-xl border-2 border-pink-200 relative overflow-hidden"
    >
      <AnimatePresence>
        {showHearts && (
          <>
            {[...Array(10)].map((_, i) => (
              <motion.div
                key={i}
                initial={{ opacity: 1, y: 0, x: 0 }}
                animate={{
                  opacity: 0,
                  y: -200,
                  x: (Math.random() - 0.5) * 200
                }}
                exit={{ opacity: 0 }}
                transition={{ duration: 2, delay: i * 0.1 }}
                className="absolute top-1/2 left-1/2 pointer-events-none"
              >
                <Heart className="w-8 h-8 text-pink-500 fill-pink-500" />
              </motion.div>
            ))}
          </>
        )}
      </AnimatePresence>

      <div className="flex items-center gap-3 mb-6">
        <div className="bg-gradient-to-br from-pink-400 to-purple-400 p-3 rounded-2xl">
          <Sparkles className="w-6 h-6 text-white" />
        </div>
        <h2 className="text-pink-800">Kirim Pesan</h2>
      </div>

      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
          <label className="block text-sm text-pink-700 mb-2">
            Pesan Cinta Kamu
          </label>
          <textarea
            value={message}
            onChange={(e) => setMessage(e.target.value)}
            placeholder="Tulis pesan romantis untuk dia..."
            rows={4}
            className="w-full px-4 py-3 rounded-xl border-2 border-pink-200 focus:border-pink-400 focus:outline-none bg-white/50 resize-none transition-colors"
          />
        </div>

        <motion.button
          whileHover={{ scale: 1.02 }}
          whileTap={{ scale: 0.98 }}
          type="submit"
          disabled={!message.trim()}
          className="w-full bg-gradient-to-r from-pink-500 to-purple-500 text-white py-3 rounded-xl flex items-center justify-center gap-2 shadow-lg disabled:opacity-50 disabled:cursor-not-allowed"
        >
          <Send className="w-5 h-5" />
          Kirim ke HeartBox
        </motion.button>
      </form>
    </motion.div>
  );
}