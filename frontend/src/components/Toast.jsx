export default function Toast({ message }) {
  if (!message) return null;

  return (
    <div className="fixed bottom-7 left-1/2 z-200 -translate-x-1/2 whitespace-nowrap rounded-[10px] bg-brand-dark px-6 py-3 text-sm font-medium text-white shadow-toast animate-fade-up">
      {message}
    </div>
  );
}
