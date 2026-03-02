#!/usr/bin/env python3
"""
Mock AI Service for Testing

Simulates an AI inference service using Python's built-in http.server.

Usage:
    python3 test_mock_ai_server.py

Then test with:
    ./duckdb -unsigned -c "LOAD 'test/extension/ai.duckdb_extension'; SELECT ai_filter('test', 'cat', 'clip');"
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import urllib.parse

class MockAIHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status_code=200, content_type='application/json'):
        self.send_response(status_code)
        self.send_header('Content-Type', content_type)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

    def do_GET(self):
        if self.path == '/health':
            self._set_headers(200)
            response = json.dumps({'status': 'healthy', 'service': 'mock-ai-service'}).encode()
            self.wfile.write(response)
        else:
            self._set_headers(404)
            self.wfile.write(b'Not Found')

    def do_POST(self):
        if self.path == '/api/v1/similarity':
            try:
                # Read request body
                content_length = int(self.headers['Content-Length'])
                post_data = self.rfile.read(content_length)
                data = json.loads(post_data.decode('utf-8'))

                image = data.get('image', '')
                prompt = data.get('prompt', '')
                model = data.get('model', 'clip')

                # Simulate AI inference with deterministic scoring
                score = 0.5
                for char in prompt:
                    score = (score * 31.0 + ord(char)) / 32.0
                score = max(0.0, min(1.0, score))  # Clamp to [0, 1]

                response_data = {
                    'score': round(score, 5),
                    'prompt': prompt,
                    'model': model,
                    'latency_ms': 50,
                    'mock': True
                }

                self._set_headers(200)
                self.wfile.write(json.dumps(response_data).encode())

            except Exception as e:
                self._set_headers(500)
                error_response = {'error': str(e)}
                self.wfile.write(json.dumps(error_response).encode())
        else:
            self._set_headers(404)
            self.wfile.write(b'Not Found')

    def log_message(self, format, *args):
        # Suppress default logging
        pass

def run_server(port=8000):
    print("🚀 Mock AI Service Starting...")
    print(f"📍 URL: http://localhost:{port}")
    print("🔍 Endpoint: POST /api/v1/similarity")
    print("💡 Health: GET /health")
    print()
    print("Test with DuckDB:")
    print("  ./duckdb -unsigned -c \\")
    print("    \"LOAD 'test/extension/ai.duckdb_extension'; \\")
    print("     SELECT ai_filter('image', 'cat', 'clip');\"")
    print()
    print("Press Ctrl+C to stop")
    print("=" * 60)

    server = HTTPServer(('0.0.0.0', port), MockAIHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Server stopped")
        server.shutdown()

if __name__ == '__main__':
    run_server()
