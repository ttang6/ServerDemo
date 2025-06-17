// 会话管理类
class ChatSession {
    constructor() {
        this.conversationId = this.generateConversationId();
        this.messages = [];
        this.loading = false;
        this.initEventListeners();
        this.loadSessionHistory();
    }

    // 生成会话ID
    generateConversationId() {
        return 'session_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
    }

    // 初始化事件监听
    initEventListeners() {
        const sendButton = document.getElementById('sendButton');
        const messageInput = document.getElementById('messageInput');
        const historyButton = document.getElementById('historyButton');

        sendButton.addEventListener('click', () => this.sendMessage());
        messageInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.sendMessage();
            }
        });
        historyButton.addEventListener('click', () => this.toggleHistory());
    }

    // 发送消息
    async sendMessage() {
        const messageInput = document.getElementById('messageInput');
        const message = messageInput.value.trim();
        
        if (!message || this.loading) return;

        this.loading = true;
        this.updateUI(message);
        messageInput.value = '';

        try {
            const response = await fetch('/api/message', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    message: message,
                    conversationId: this.conversationId
                })
            });

            const data = await response.json();
            this.addMessage('assistant', data.content);
            this.messages.push({ role: 'user', content: message });
            this.messages.push({ role: 'assistant', content: data.content });
        } catch (error) {
            console.error('Error:', error);
            this.addMessage('system', '发送消息失败，请重试');
        } finally {
            this.loading = false;
        }
    }

    // 加载会话历史
    async loadSessionHistory() {
        try {
            const response = await fetch(`/api/session/${this.conversationId}`);
            if (response.ok) {
                const data = await response.json();
                this.messages = data.messages.map(msg => ({
                    role: 'user',
                    content: msg
                }));
                if (data.lastResponse) {
                    this.messages.push({
                        role: 'assistant',
                        content: data.lastResponse
                    });
                }
                this.renderMessages();
            }
        } catch (error) {
            console.error('Error loading session history:', error);
        }
    }

    // 更新UI
    updateUI(message) {
        this.addMessage('user', message);
        this.addMessage('system', '正在思考...');
    }

    // 添加消息到界面
    addMessage(role, content) {
        const chatBox = document.getElementById('chatBox');
        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${role}`;
        
        const contentDiv = document.createElement('div');
        contentDiv.className = 'message-content';
        contentDiv.textContent = content;
        
        messageDiv.appendChild(contentDiv);
        chatBox.appendChild(messageDiv);
        chatBox.scrollTop = chatBox.scrollHeight;
    }

    // 渲染所有消息
    renderMessages() {
        const chatBox = document.getElementById('chatBox');
        chatBox.innerHTML = '';
        this.messages.forEach(msg => {
            this.addMessage(msg.role, msg.content);
        });
    }

    // 切换历史记录显示
    toggleHistory() {
        const historyPanel = document.getElementById('historyPanel');
        historyPanel.classList.toggle('show');
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    window.chatSession = new ChatSession();
}); 